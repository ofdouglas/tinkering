#include <algorithm>
#include <cstring>

#include "stm32f746xx.h"

#include "ethernet.h"
#include "logging/logging.h"
#include "osal/critical_section.h"

Ethernet *Ethernet::instance_{nullptr};

void Ethernet::EthDmaTxDescriptor::setBufferSizes(uint16_t buffer_size_1,
                                                  uint16_t buffer_size_2) noexcept {
    buffer_size_1 &= 0x1FFFU;
    buffer_size_2 &= 0x1FFFU;
    byte_counts = (static_cast<uint32_t>(buffer_size_2) << 16U) | buffer_size_1;
}

bool Ethernet::EthDmaTxDescriptor::isOwnedByDma() const noexcept {
    return (status_control & kStatusOwn) != 0U;
}

uint16_t Ethernet::phyTransaction(uint8_t phy_address, uint8_t mii_register, uint16_t data,
                                  bool is_write) noexcept {
    // Wait for any previous request to finish.
    while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0U) {
    }

    uint32_t request{((phy_address & 0x1FU) << ETH_MACMIIAR_PA_Pos) | ETH_MACMIIAR_CR_Div102 |
                     ((mii_register & 0x1FU) << ETH_MACMIIAR_MR_Pos) | ETH_MACMIIAR_MB};

    if (is_write) {
        ETH->MACMIIDR = data;
        request |= ETH_MACMIIAR_MW;
    }

    ETH->MACMIIAR = request;

    while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0U) {
    }
    return static_cast<uint16_t>(ETH->MACMIIDR);
}

uint16_t Ethernet::phyRead(uint8_t phy_address, uint8_t mii_register) noexcept {
    return phyTransaction(phy_address, mii_register, 0U, false);
}

void Ethernet::phyWrite(uint8_t phy_address, uint8_t mii_register, uint16_t data) noexcept {
    (void)phyTransaction(phy_address, mii_register, data, true);
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

bool Ethernet::send(util::Span<const uint8_t> payload) noexcept {
    EthDmaTxDescriptor *const descriptor{allocateTxDescriptor()};
    if (descriptor == nullptr) {
        LOG_ERROR() << "Ethernet TX allocate failed";
        return false;
    }

    // TODO: zero-copy (later)
    const size_t copy_size{std::min(kMaxEthernetFrameSize, payload.size())};
    descriptor->setBufferSizes(static_cast<uint16_t>(copy_size), 0U);
    std::memcpy(descriptor->buffer_ptr, payload.data(), copy_size);
    __DMB();
    descriptor->status_control |= EthDmaTxDescriptor::kStatusOwn;
    ETH->DMATPDR = 0U;

    return true;
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
            pending_index = (pending_index + 1U) % kNumTxBuffers;
            descriptor = &tx_descriptors_[pending_index];
            pending_index_.store(pending_index);
        }
    }

    // Clear all flags. TODO: only clear the flags handled above.
    ETH->DMASR = status | 0x1'E7FFU;
}

void Ethernet::init() noexcept {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->APB2ENR;

    SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;

    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN | RCC_AHB1ENR_ETHMACEN;
    RCC->AHB1ENR;

    ETH->DMABMR |= ETH_DMABMR_SR;
    while ((ETH->DMABMR & ETH_DMABMR_SR) != 0U) {
    }

    for (size_t i{0U}; i < kNumTxBuffers; ++i) {
        const size_t next_index{(i + 1U) % kNumTxBuffers};
        tx_descriptors_[i].status_control = kTxDescriptorFlags;
        tx_descriptors_[i].setBufferSizes(0U, 0U);
        tx_descriptors_[i].buffer_ptr = tx_buffers_[i].data();
        tx_descriptors_[i].next_ptr = &tx_descriptors_[next_index];
    }
    ETH->DMATDLAR = reinterpret_cast<uint32_t>(tx_descriptors_.data());
    ETH->DMAOMR |= ETH_DMAOMR_ST;

    const uint16_t phy_id_1{phyRead(kPhyAddress, Lan8742a::RegAddress::kPhyId1)};
    const uint16_t phy_id_2{static_cast<uint16_t>(
        phyRead(kPhyAddress, Lan8742a::RegAddress::kPhyId2) & Lan8742a::kExpectedPhyId2Mask)};
    if ((phy_id_1 != Lan8742a::kExpectedPhyId1) || (phy_id_2 != Lan8742a::kExpectedPhyId2)) {
        LOG_ERROR() << "Read PHY Id: " << logging::Radix::Hexadecimal << phy_id_2 << ":"
                    << logging::Radix::Hexadecimal << phy_id_1;
        return;
    }

    // Spin until autonegotiation is complete and the link is up. TODO: timeouts
    bool negotiation_complete{false};
    bool link_up{false};
    do {
        const uint16_t phy_status{phyRead(kPhyAddress, Lan8742a::RegAddress::kBasicStatus)};
        negotiation_complete =
            (phy_status & Lan8742a::BasicStatus::kAutoNegotiateCompleteBit) != 0U;
        link_up = (phy_status & Lan8742a::BasicStatus::kLinkStatusBit) != 0U;
    } while (!link_up || !negotiation_complete);

    const uint16_t partner_ability{
        phyRead(kPhyAddress, Lan8742a::RegAddress::kAutoNegotiationPartnerAbility)};
    if ((partner_ability & Lan8742a::PartnerAbility::k100BaseTxFullDuplex) == 0U) {
        LOG_ERROR() << "100Base-TX Full Duplex is not supported.";
        return;
    }

    // 100 Mbit/s, full duplex, receive all frames in promiscuous mode.
    ETH->MACFFR |= ETH_MACFFR_RA | ETH_MACFFR_PM;
    ETH->DMAOMR |= ETH_DMAOMR_RSF | ETH_DMAOMR_TSF | ETH_DMAOMR_FEF | ETH_DMAOMR_FUGF;
    ETH->DMABMR |= ETH_DMABMR_PBL_32Beat;
    ETH->DMAIER |=
        ETH_DMAIER_NISE | ETH_DMAIER_TIE | ETH_DMAIER_AISE | ETH_DMAIER_FBEIE | ETH_DMAIER_TBUIE;

    // Enable transmit. TODO: enable receive once its DMA path is ready.
    ETH->MACCR |= ETH_MACCR_FES | ETH_MACCR_DM | ETH_MACCR_TE;

    instance_ = this;
    NVIC_SetPriority(ETH_IRQn, 2U);
    NVIC_EnableIRQ(ETH_IRQn);
}

extern "C" void ETH_IRQHandler() {
    Ethernet::handleInterrupt();
}
