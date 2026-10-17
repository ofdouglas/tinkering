#include <algorithm>
#include <cstring>

#include "stm32f746xx.h"

#include "ethernet.h"
#include "logging/logging.h"
#include "hal/poll.h"
#include "osal/critical_section.h"

///////////////////////////////////////////////////////////////////////////////
// Interrupt handling
///////////////////////////////////////////////////////////////////////////////

Ethernet *Ethernet::instance_{nullptr};

extern "C" void ETH_IRQHandler() {
    Ethernet::handleInterrupt();
}

void Ethernet::handleInterrupt() noexcept {
    if (instance_ != nullptr) {
        instance_->handleInterruptInstance();
    }
}

void Ethernet::handleInterruptInstance() noexcept {
    const uint32_t status{ETH->DMASR};
    const uint32_t error_status{(status & ETH_DMASR_EBS_Msk) >> ETH_DMASR_EBS_Pos};
    const uint32_t tx_state{(status & ETH_DMASR_TPS_Msk) >> ETH_DMASR_TPS_Pos};
    const bool tx_underflow{(status & ETH_DMASR_TUS) != 0U};
    const bool tx_jabber_timeout{(status & ETH_DMASR_TJTS) != 0U};
    const bool tx_no_buffers{(status & ETH_DMASR_TBUS) != 0U};
    const bool tx_finished{(status & ETH_DMASR_TS) != 0U};

    if ((status & ETH_DMASR_AIS) != 0U) {
        LOG_ERROR() << "[ETH TX] err_status = " << error_status << ", tx_state = " << tx_state
                    << (tx_underflow ? ", underflow" : "") << (tx_no_buffers ? ", no buffers" : "")
                    << (tx_jabber_timeout ? ", jabber timeout" : "")
                    << ", status = " << logging::Radix::Hexadecimal << status;
    }

    if (tx_finished) {
        uint32_t pending_index{pending_index_.load()};
        EthDmaTxDescriptor *descriptor{&tx_descriptors_[pending_index]};

        while ((pending_index != free_index_.load()) && !descriptor->isOwnedByDma()) {
            packet_buffer_handles_[pending_index].reset();

            pending_index = (pending_index == kNumTxBuffers - 1U) ? 0U : (pending_index + 1U);
            pending_index_.store(pending_index);
            descriptor = &tx_descriptors_[pending_index];
        }
    }

    // Clear all flags. TODO: only clear the flags handled above.
    ETH->DMASR = status | 0x1'E7FFU;
}

///////////////////////////////////////////////////////////////////////////////
// DMA and packet buffer management
///////////////////////////////////////////////////////////////////////////////

void Ethernet::EthDmaTxDescriptor::setBufferSizes(uint16_t buffer_size_1,
                                                  uint16_t buffer_size_2) noexcept {
    buffer_size_1 &= 0x1FFFU;
    buffer_size_2 &= 0x1FFFU;
    byte_counts = (static_cast<uint32_t>(buffer_size_2) << 16U) | buffer_size_1;
}

bool Ethernet::EthDmaTxDescriptor::isOwnedByDma() const noexcept {
    return (status_control & kStatusOwn) != 0U;
}

Ethernet::EthDmaTxDescriptor *Ethernet::allocateTxDescriptor() noexcept {
    osal::RaiiCriticalSection critical_section{};

    const uint32_t index{free_index_.load()};
    if (index >= kNumTxBuffers) {
        LOG_ERROR() << "Ethernet TX buffer index out of range";
        return nullptr;
    }

    const uint32_t next_index{(index + 1U) % kNumTxBuffers};
    if (tx_descriptors_[next_index].isOwnedByDma()) {
        return nullptr;
    }

    free_index_.store(next_index);
    return &tx_descriptors_[index];
}

Ethernet::PacketBufferHandle Ethernet::allocatePacketBuffer() noexcept {
    return packet_pool_.allocate();
}

bool Ethernet::send(PacketBufferHandle &&packet_handle) noexcept {
    if (state_.load() != State::kLinkUp) {
        LOG_ERROR() << "TX failed: link down";
        return false;
    }

    EthDmaTxDescriptor *const descriptor{allocateTxDescriptor()};
    if (descriptor == nullptr) {
        LOG_ERROR() << "Ethernet TX descriptor allocation failed";
        return false;
    }

    packet_buffer_handles_[descriptor->index] = std::move(packet_handle);
    auto &handle{packet_buffer_handles_[descriptor->index]};

    descriptor->setBufferSizes(static_cast<uint16_t>(handle->length), 0U);
    descriptor->buffer_ptr = handle->bytes.data();
    __DMB();
    descriptor->status_control |= EthDmaTxDescriptor::kStatusOwn;
    ETH->DMATPDR = 0U;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// PHY
///////////////////////////////////////////////////////////////////////////////

uint16_t Ethernet::phyRead(uint8_t phy_address, uint8_t mii_register) noexcept {
    return phyTransaction(phy_address, mii_register, 0U, false);
}

void Ethernet::phyWrite(uint8_t phy_address, uint8_t mii_register, uint16_t data) noexcept {
    (void)phyTransaction(phy_address, mii_register, data, true);
}

uint16_t Ethernet::phyTransaction(uint8_t phy_address, uint8_t mii_register, uint16_t data,
                                  bool is_write) noexcept {
    static constexpr auto kPhyTransactionTimeout{std::chrono::milliseconds(25U)};

    if (!hal::pollOrLogError<hal::SchedulerClock>(
            "previous PHY transaction", kPhyTransactionTimeout,
            []() { return (ETH->MACMIIAR & ETH_MACMIIAR_MB) == 0U; })) {
        return 0U;  // TODO: std::optional<uint16_t> or equivalent
    }

    uint32_t request{((phy_address & 0x1FU) << ETH_MACMIIAR_PA_Pos) | ETH_MACMIIAR_CR_Div102 |
                     ((mii_register & 0x1FU) << ETH_MACMIIAR_MR_Pos) | ETH_MACMIIAR_MB};

    if (is_write) {
        ETH->MACMIIDR = data;
        request |= ETH_MACMIIAR_MW;
    }

    ETH->MACMIIAR = request;

    if (!hal::pollOrLogError<hal::SchedulerClock>(
            "current PHY transaction", kPhyTransactionTimeout,
            []() { return (ETH->MACMIIAR & ETH_MACMIIAR_MB) == 0U; })) {
        return 0U;  // TODO: std::optional<uint16_t> or equivalent
    }
    return static_cast<uint16_t>(ETH->MACMIIDR);
}

bool Ethernet::phyVerifyPhyId() noexcept {
    const uint16_t phy_id_1{phyRead(kPhyAddress, Lan8742a::RegAddress::kPhyId1)};
    const uint16_t phy_id_2{static_cast<uint16_t>(
        phyRead(kPhyAddress, Lan8742a::RegAddress::kPhyId2) & Lan8742a::kExpectedPhyId2Mask)};

    if ((phy_id_1 != Lan8742a::kExpectedPhyId1) || (phy_id_2 != Lan8742a::kExpectedPhyId2)) {
        LOG_ERROR() << "Read PHY Id: " << logging::Radix::Hexadecimal << phy_id_2 << ":"
                    << logging::Radix::Hexadecimal << phy_id_1;
        return false;
    }
    return true;
}

bool Ethernet::pollAutoNegotiationComplete() noexcept {
    const uint16_t phy_status{phyRead(kPhyAddress, Lan8742a::RegAddress::kBasicStatus)};
    return (phy_status & Lan8742a::BasicStatus::kAutoNegotiateCompleteBit) != 0U;
}

bool Ethernet::verifyLinkConfiguration() noexcept {
    const uint16_t partner_ability{
        phyRead(kPhyAddress, Lan8742a::RegAddress::kAutoNegotiationPartnerAbility)};
    if ((partner_ability & Lan8742a::PartnerAbility::k100BaseTxFullDuplex) == 0U) {
        LOG_ERROR() << "Auto-negotiation failed: 100Base-TX Full Duplex is not supported.";
        return false;
    }
    return true;
}

bool Ethernet::pollLinkStatus() noexcept {
    const uint16_t phy_status{phyRead(kPhyAddress, Lan8742a::RegAddress::kBasicStatus)};
    return (phy_status & Lan8742a::BasicStatus::kLinkStatusBit) != 0U;
}

///////////////////////////////////////////////////////////////////////////////
// State machine and initialization
///////////////////////////////////////////////////////////////////////////////

Ethernet::State Ethernet::runLinkManagement() noexcept {
    State next_state{state_.load()};

    switch (next_state) {
        case State::kUninitialized:
            LOG_ERROR() << "Ethernet driver not initialized";
            break;

        case State::kAutoNegotiation:
            if (!pollAutoNegotiationComplete()) {
                break;
            }
            if (verifyLinkConfiguration()) {
                next_state = State::kLinkDown;
                LOG_INFO() << "Auto-negotiation complete";
                break;
            }
            next_state = State::kError;
            LOG_ERROR() << "Auto-negotiation failed: link configuration verification failed";
            break;

        case State::kLinkUp:
            if (!pollLinkStatus()) {
                next_state = State::kLinkDown;
                LOG_INFO() << "Link down";

                // TODO: confirm it's safe to disable TX/RX (what happens to in-process)
                // frames and DMA transfers?
                disableTxRx();
            }
            break;

        case State::kLinkDown:
            if (pollLinkStatus()) {
                next_state = State::kLinkUp;
                LOG_INFO() << "Link up";
                enableTxRx();
            }
            break;

        case State::kError:
            // Terminal error state.
            disableTxRx();
            break;

        default:
            break;
    }
    state_.store(next_state);
    return next_state;
}

void Ethernet::enableTxRx() noexcept {
    ETH->MACCR |= ETH_MACCR_TE;  // TODO: enable RX once its DMA path is ready.
    ETH->DMAOMR |= ETH_DMAOMR_ST;
}

void Ethernet::disableTxRx() noexcept {
    ETH->MACCR &= ~ETH_MACCR_TE;  // TODO: disable RX once its DMA path is ready.
    ETH->DMAOMR &= ~ETH_DMAOMR_ST;
}

Ethernet::State Ethernet::getState() const noexcept {
    return state_.load();
}

bool Ethernet::softwareResetMac() noexcept {
    ETH->DMABMR |= ETH_DMABMR_SR;
    return hal::pollOrLogError<hal::SchedulerClock>(
        "software reset MAC", hal::kDefaultPollingTimeout,
        []() { return (ETH->DMABMR & ETH_DMABMR_SR) == 0U; });
}

void Ethernet::init() noexcept {
    // Enable clocks, and select RMII mode.
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->APB2ENR;
    SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;
    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN | RCC_AHB1ENR_ETHMACEN;
    RCC->AHB1ENR;

    if (!softwareResetMac()) {
        LOG_ERROR() << "Ethernet not initialized: software reset MAC failed";
        return;
    }

    if (!phyVerifyPhyId()) {
        LOG_ERROR() << "Ethernet not initialized: PHY ID verification failed";
        return;
    }

    // Set the instance pointer used for interrupt dispatch.
    instance_ = this;

    // Initialize the transmit DMA ring.
    for (size_t i{0U}; i < kNumTxBuffers; ++i) {
        const size_t next_index{(i + 1U) % kNumTxBuffers};
        tx_descriptors_[i].status_control = kTxDescriptorFlags;
        tx_descriptors_[i].setBufferSizes(0U, 0U);
        tx_descriptors_[i].buffer_ptr = nullptr;
        tx_descriptors_[i].next_ptr = &tx_descriptors_[next_index];
        tx_descriptors_[i].index = i;
    }
    ETH->DMATDLAR = reinterpret_cast<uint32_t>(tx_descriptors_.data());

    // Receive all, promiscuous mode.
    ETH->MACFFR |= ETH_MACFFR_RA | ETH_MACFFR_PM;
    // Store-and-forward mode, forward all frames.
    ETH->DMAOMR |= ETH_DMAOMR_RSF | ETH_DMAOMR_TSF | ETH_DMAOMR_FEF | ETH_DMAOMR_FUGF;
    ETH->DMABMR |= ETH_DMABMR_PBL_32Beat;
    // 100 Mbit/s, full duplex. TODO: support other link type based on PHY status.
    ETH->MACCR |= ETH_MACCR_FES | ETH_MACCR_DM;
    // Enable TX interrupts and error interrupts.
    ETH->DMAIER |=
        ETH_DMAIER_NISE | ETH_DMAIER_TIE | ETH_DMAIER_AISE | ETH_DMAIER_FBEIE | ETH_DMAIER_TBUIE;

    state_.store(State::kAutoNegotiation);

    NVIC_SetPriority(ETH_IRQn, 2U);
    NVIC_EnableIRQ(ETH_IRQn);
}
