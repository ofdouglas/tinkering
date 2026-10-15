#include <algorithm>
#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>

#include "stm32f746xx.h"
#include "core_cm7.h"

#include "bare_metal/task.h"
#include "data_structures/ring_buffer.h"
#include "logging/logging.h"
#include "hal/clock.h"
#include "hal/delay.h"
#include "util/span.h"

std::atomic<uint32_t> sys_tick_count{};

extern "C" void SysTick_Handler() {
  sys_tick_count++;
}

hal::SchedulerClock::TimePoint hal::SchedulerClock::now() noexcept {
  return hal::SchedulerClock::TimePoint(std::chrono::milliseconds(sys_tick_count));
}

void bsp_init_systick() {
  SysTick->LOAD = 168000;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void bsp_init_watchdog() {
  IWDG->KR = 0x5555;
  IWDG->KR = 0xCCCC;
}

void bsp_watchdog_kick() {
  IWDG->KR = 0xAAAA;
}

void bsp_init_gpio() {
  RCC->AHB1ENR = RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOGEN;
  RCC->AHB1ENR;

  // USART1: PA9 and PB7, both use alt. function 7
  GPIOA->AFR[1] = GPIO_AFRH_AFRH1_2 | GPIO_AFRH_AFRH1_1 | GPIO_AFRH_AFRH1_0;
  GPIOB->AFR[0] = GPIO_AFRL_AFRL7_2 | GPIO_AFRL_AFRL7_1 | GPIO_AFRL_AFRL7_0;
  GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9) | GPIO_MODER_MODER9_1;
  GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7) | GPIO_MODER_MODER7_1;

  // TODO: LED PWM: PB4, alt. function 2 (TIM1 ch.1)
  // GPIOB->AFR[0] |= GPIO_AFRL_AFRL4_1;
  // GPIOB->MODER |= GPIO_MODER_MODER4_0;

  // PC7 : output to LED (TODO: remove)
  GPIOC->MODER = GPIO_MODER_MODER7_0;

  /* Ethernet pins (all use alternate function 11)
    * = high drive strength
    PG2 = RMII_RXER
    PG3 = EXT_RST
    PG11 = ETH_TX_EN*
    PG13 = ETH_TXD0*
    PG14 = ETH_TXD1*
    PC1 = ETH_MDC
    PC4 = ETH_RXD0
    PC5 = ETH_RXD1
    PA1 = ETH_REF_CLK
    PA2 = ETH_MDIO
    PA7 = ETH_CRS_DV
  */
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER1_Msk | GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER7_Msk))
                 | GPIO_MODER_MODER1_1 | GPIO_MODER_MODER2_1 | GPIO_MODER_MODER7_1;
  GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(GPIO_AFRL_AFRL1_Msk | GPIO_AFRL_AFRL2_Msk | GPIO_AFRL_AFRL7_Msk))
                  | GPIO_AFRL_AFRL1_0 | GPIO_AFRL_AFRL1_1 | GPIO_AFRL_AFRL1_3 | GPIO_AFRL_AFRL2_0
                  | GPIO_AFRL_AFRL2_1 | GPIO_AFRL_AFRL2_3 | GPIO_AFRL_AFRL7_0 | GPIO_AFRL_AFRL7_1
                  | GPIO_AFRL_AFRL7_3;

  GPIOC->MODER = (GPIOC->MODER & ~(GPIO_MODER_MODER1_Msk | GPIO_MODER_MODER4_Msk | GPIO_MODER_MODER5_Msk))
                 | GPIO_MODER_MODER1_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1;
  GPIOC->AFR[0] = (GPIOC->AFR[0] & ~(GPIO_AFRL_AFRL1_Msk | GPIO_AFRL_AFRL4_Msk | GPIO_AFRL_AFRL5_Msk))
                  | GPIO_AFRL_AFRL1_0 | GPIO_AFRL_AFRL1_1 | GPIO_AFRL_AFRL1_3 | GPIO_AFRL_AFRL4_0
                  | GPIO_AFRL_AFRL4_1 | GPIO_AFRL_AFRL4_3 | GPIO_AFRL_AFRL5_0 | GPIO_AFRL_AFRL5_1
                  | GPIO_AFRL_AFRL5_3;

  GPIOG->MODER = (GPIOG->MODER
                  & ~(GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER11_Msk | GPIO_MODER_MODER13_Msk
                      | GPIO_MODER_MODER14_Msk))
                 | GPIO_MODER_MODER2_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER13_1
                 | GPIO_MODER_MODER14_1;
  GPIOG->AFR[0] = (GPIOG->AFR[0] & ~GPIO_AFRL_AFRL2_Msk) | GPIO_AFRL_AFRL2_0 | GPIO_AFRL_AFRL2_1
                  | GPIO_AFRL_AFRL2_3;
  GPIOG->AFR[1] = (GPIOG->AFR[1]
                   & ~(GPIO_AFRH_AFRH3_Msk | GPIO_AFRH_AFRH5_Msk | GPIO_AFRH_AFRH6_Msk))
                  | GPIO_AFRH_AFRH3_0 | GPIO_AFRH_AFRH3_1 | GPIO_AFRH_AFRH3_3 | GPIO_AFRH_AFRH5_0
                  | GPIO_AFRH_AFRH5_1 | GPIO_AFRH_AFRH5_3 | GPIO_AFRH_AFRH6_0 | GPIO_AFRH_AFRH6_1
                  | GPIO_AFRH_AFRH6_3;
  GPIOG->OSPEEDR = GPIO_OSPEEDR_OSPEEDR13_Msk | GPIO_OSPEEDR_OSPEEDR14_Msk | GPIO_OSPEEDR_OSPEEDR11_Msk;
}

uint16_t bsp_eth_phy_transaction(uint8_t phy_address, uint8_t mii_register, uint16_t data, bool is_write) {
  // Wait for any previous request to finish
  while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0U) {}

  uint32_t request = ((phy_address & 0x1F) << ETH_MACMIIAR_PA_Pos) |
                     ((mii_register & 0x1F) << ETH_MACMIIAR_MR_Pos) |
                     ETH_MACMIIAR_CR_Div102 | ETH_MACMIIAR_MB;

  if (is_write) {
    ETH->MACMIIDR = data;
    request |= ETH_MACMIIAR_MW;
  }

  ETH->MACMIIAR = request;

  // Read DR once the new request finishes
  while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0U) {}
  return ETH->MACMIIDR;
}

uint16_t bsp_eth_phy_read(uint8_t phy_address, uint8_t mii_register) {
  return bsp_eth_phy_transaction(phy_address, mii_register, 0U, false);
}

void bsp_eth_phy_write(uint8_t phy_address, uint8_t mii_register, uint16_t data) {
  bsp_eth_phy_transaction(phy_address, mii_register, data, true);
}

struct Lan8742a {
  struct RegAddress {
    static constexpr uint16_t kBasicControl = 0;
    static constexpr uint16_t kBasicStatus = 1;
    static constexpr uint16_t kPhyId1 = 2;
    static constexpr uint16_t kPhyId2 = 3;
    static constexpr uint16_t kAutoNegotiationAdvertise = 4;
    static constexpr uint16_t kAutoNegotiationPartnerAbility = 5;
    // Other registers omitted for now
  };

  struct BasicStatus {
    static constexpr uint16_t kExtendedCapabilities = (1U << 0U);
    static constexpr uint16_t kJabberDetectBit = (1U << 1U);
    static constexpr uint16_t kLinkStatusBit = (1U << 2U);
    static constexpr uint16_t kRemoteFaultBit = (1U << 4U);
    static constexpr uint16_t kAutoNegotiateCompleteBit = (1U << 5U);
    static constexpr uint16_t kExtendedStatus = (1U << 8U);
    // Other bits omitted for now
  };

  struct PartnerAbility {
    static constexpr uint16_t k10BaseT = (1U << 5U);
    static constexpr uint16_t k10BaseT_Fd = (1U << 6U);
    static constexpr uint16_t k100BaseTX = (1U << 7U);
    static constexpr uint16_t k100BaseTX_Fd = (1U << 8U);
    // Other bits omitted for now
  };

  static constexpr uint16_t kExpectedPhyId1 = 0x0007U;
  static constexpr uint16_t kExpectedPhyId2 = 0xC130U;
  // The low 4 bits of PhyId2 are a revision number. Don't check them.
  static constexpr uint16_t kExpectedPhyId2Mask = 0xFFF0U;
};

constexpr uint8_t kPhyAddress = 0U;

struct EthDmaTxDescriptor {
  // Control bits
  static constexpr uint32_t kTdes0_StatusOwn            = (1U << 31);
  static constexpr uint32_t kTdes0_InterruptOnComplete  = (1U << 30);
  static constexpr uint32_t kTdes0_LastSegment          = (1U << 29);
  static constexpr uint32_t kTdes0_FirstSegment         = (1U << 28);
  static constexpr uint32_t kTdes0_DisableCrc           = (1U << 27);
  static constexpr uint32_t kTdes0_DisablePad           = (1U << 26);
  static constexpr uint32_t kTdes0_TxTimestampEnable    = (1U << 25);
  static constexpr uint32_t kTdes0_ChecksumInsertion_0  = (1U << 24);
  static constexpr uint32_t kTdes0_ChecksumInsertion_1  = (1U << 23);
  static constexpr uint32_t kTdes0_TxEndOfRing          = (1U << 21);
  static constexpr uint32_t kTdes0_SecondAddressChained = (1U << 20);
  // Status bits
  static constexpr uint32_t kTdes0_TxTimeStampStatus    = (1U << 17);
  static constexpr uint32_t kTdes0_IpHeaderError        = (1U << 16);
  static constexpr uint32_t kTdes0_ErrorSummary         = (1U << 15);
  static constexpr uint32_t kTdes0_JabberTimeout        = (1U << 14);
  static constexpr uint32_t kTdes0_FrameFlushed         = (1U << 13);
  static constexpr uint32_t kTdes0_IpPayloadError       = (1U << 12);
  static constexpr uint32_t kTdes0_LossOfCarrier        = (1U << 11);
  static constexpr uint32_t kTdes0_NoCarrier            = (1U << 10);
  static constexpr uint32_t kTdes0_LateCollision        = (1U << 9);
  static constexpr uint32_t kTdes0_ExcessiveCollision   = (1U << 8);
  static constexpr uint32_t kTdes0_VlanFrame            = (1U << 7);
  static constexpr uint8_t kTdes0_CollisionCount_Mask   = 0x78U;
  static constexpr uint8_t kTdes0_CollisionCount_Offset = 3U;
  static constexpr uint32_t kTdes0_ExcessiveDeferral    = (1U << 2);
  static constexpr uint32_t kTdes0_UnderflowError       = (1U << 1);
  static constexpr uint32_t kTdes0_DeferredTransmit     = (1U << 0);

  void setBufferSizes(uint16_t tx_buf_size_1, uint16_t tx_buf_size_2) {
    tx_buf_size_1 &= 0x1FFFU;
    tx_buf_size_2 &= 0x1FFFU;
    tdes1_byte_counts = (tx_buf_size_2 << 16U) | tx_buf_size_1;
  }

  uint32_t tdes0_status_ctrl{};
  uint32_t tdes1_byte_counts{};
  uint8_t* tdes2_buffer_ptr{};
  EthDmaTxDescriptor* tdes3_next_ptr{};
};

// Add the node at the end of the singly-linked list
void bsp_ethernet_tx_list_append(std::atomic<EthDmaTxDescriptor*>* list_head, EthDmaTxDescriptor* node) {
  node->tdes3_next_ptr = nullptr;

  if (list_head == nullptr) {
    LOG_ERROR() << "nullptr";
    return;
  }

  if (*list_head == nullptr) {
    *list_head = node;
    return;
  }

  EthDmaTxDescriptor* current = *list_head;
  while (current->tdes3_next_ptr != nullptr) {
    current = current->tdes3_next_ptr;
  }
  current->tdes3_next_ptr = node;
}


constexpr size_t kNumEthTxBuffers = 8U;
constexpr size_t kMaxEthernetFrameSize = 1518U;
constexpr size_t kEthTxBufferSize = kMaxEthernetFrameSize;


// Must be word aligned
alignas(4) std::array<EthDmaTxDescriptor, kNumEthTxBuffers> eth_tx_descriptors;

// Cache line size alignment (note: D-cache is not enabled yet)
alignas(32) std::array<uint8_t[kEthTxBufferSize], kNumEthTxBuffers> eth_tx_buffers;

// CPU-owned descriptors ready to be allocated
std::atomic<EthDmaTxDescriptor*> eth_tx_free_list = nullptr;

// DMA-owned descriptors, awaiting transmission
std::atomic<EthDmaTxDescriptor*> eth_tx_pending_list = nullptr;

const uint32_t kEthTxDes0Flags = EthDmaTxDescriptor::kTdes0_InterruptOnComplete | 
                             EthDmaTxDescriptor::kTdes0_LastSegment | EthDmaTxDescriptor::kTdes0_FirstSegment; 
                             // EthDmaTxDescriptor::kTdes0_SecondAddressChained;
                             // EthDmaTxDescriptor::kTdes0_ChecksumInsertion_1 

EthDmaTxDescriptor* bsp_ethernet_tx_allocate() {
  EthDmaTxDescriptor* result = nullptr;

  __disable_irq();
  if (eth_tx_free_list != nullptr) {
    result = eth_tx_free_list;
    eth_tx_free_list = eth_tx_free_list.load()->tdes3_next_ptr;
    result->tdes3_next_ptr = nullptr;
  }
  __enable_irq();
  return result;
}

bool bsp_ethernet_send(util::Span<const uint8_t> payload) {
  EthDmaTxDescriptor* descriptor = bsp_ethernet_tx_allocate();
  if (descriptor == nullptr) {
    LOG_ERROR() << "Ethernet TX allocate failed";
    return false;
  }

  // TODO: zero-copy (later)
  const size_t copy_size = std::min(kEthTxBufferSize, payload.size());
  descriptor->setBufferSizes(copy_size, 0U);
  memcpy(descriptor->tdes2_buffer_ptr, payload.data(), copy_size);
  descriptor->tdes3_next_ptr = nullptr;
  descriptor->tdes0_status_ctrl = kEthTxDes0Flags;

  __disable_irq();
  if (eth_tx_pending_list == nullptr) {
    descriptor->tdes0_status_ctrl |= EthDmaTxDescriptor::kTdes0_TxEndOfRing;
    descriptor->tdes0_status_ctrl |= EthDmaTxDescriptor::kTdes0_StatusOwn;
    __DSB();
    eth_tx_pending_list = descriptor;
    ETH->DMATDLAR = reinterpret_cast<uint32_t>(descriptor);
    ETH->DMAOMR |= ETH_DMAOMR_ST;
  } else {
    descriptor->tdes0_status_ctrl |= EthDmaTxDescriptor::kTdes0_StatusOwn;
    __DSB();
    bsp_ethernet_tx_list_append(&eth_tx_pending_list, descriptor);
  }
  ETH->DMATPDR = 0U;
  __enable_irq();

  return true;
}


extern "C" {
void ETH_IRQHandler(void) {
  uint32_t status = ETH->DMASR;
  uint32_t err_status = (status & ETH_DMASR_EBS_Msk) >> ETH_DMASR_EBS_Pos;
  uint32_t tx_state = (status & ETH_DMASR_TPS_Msk) >> ETH_DMASR_TPS_Pos;
  bool tx_underflow = (status & ETH_DMASR_TUS) ? true : false;
  bool tx_jabber_timeout = (status & ETH_DMASR_TJTS) ? true : false;
  bool tx_no_buffers = (status & ETH_DMASR_TBUS) ? true : false;
  bool tx_stopped = (status & ETH_DMASR_TPSS) ? true : false;
  bool tx_finished = (status & ETH_DMASR_TS) ? true : false;

  if ((status & ETH_DMASR_AIS) != 0U) {
    LOG_ERROR() << "[ETH TX] err_status = " << err_status << ", tx_state = " << tx_state
                << (tx_underflow ? ", underflow" : "") << (tx_no_buffers ? ", no buffers" : "")
                << (tx_jabber_timeout ? ", jabber timeout" : "");
  }

  if (tx_finished) {
    size_t num_reclaimed = 0U;
    EthDmaTxDescriptor* descriptor = eth_tx_pending_list;

    while (descriptor != nullptr) {
      if ((descriptor->tdes0_status_ctrl & EthDmaTxDescriptor::kTdes0_StatusOwn) != 0U) {
        break;
      }

      EthDmaTxDescriptor* freed_node = descriptor;
      descriptor = descriptor->tdes3_next_ptr;
      freed_node->tdes3_next_ptr = nullptr;

      bsp_ethernet_tx_list_append(&eth_tx_free_list, freed_node);
      num_reclaimed++;
    }

    if (num_reclaimed == 0U) {
      LOG_ERROR() << "No descriptors reclaimed";
    } 
  }

  ETH->DMASR = status | 0x1'E7FFU; // Clear all flags. TODO: only clear the ones we handled
}
}

void bsp_init_ethernet() {
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  RCC->APB2ENR;

  SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL; // Use RMII

  RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN | RCC_AHB1ENR_ETHMACEN;
  RCC->AHB1ENR;

  // Reset Ethernet registers
  ETH->DMABMR |= ETH_DMABMR_SR;
  while ((ETH->DMABMR & ETH_DMABMR_SR) != 0U) {}

  // Initialize DMA descriptors
  for (size_t i = 0U; i < kNumEthTxBuffers; i++) {
    eth_tx_descriptors[i].tdes2_buffer_ptr = eth_tx_buffers[i];
    bsp_ethernet_tx_list_append(&eth_tx_free_list, &eth_tx_descriptors[i]);
  }

  // Verify the PHY is a LAN8742A.
  uint16_t phyId1 = bsp_eth_phy_read(kPhyAddress, Lan8742a::RegAddress::kPhyId1);
  uint16_t phyId2 = bsp_eth_phy_read(kPhyAddress, Lan8742a::RegAddress::kPhyId2) & Lan8742a::kExpectedPhyId2Mask;
  if ((phyId1 != Lan8742a::kExpectedPhyId1) || (phyId2 != Lan8742a::kExpectedPhyId2)) {
    LOG_ERROR() << "Read PHY Id: " << logging::Radix::Hexadecimal << phyId2 << ":" << logging::Radix::Hexadecimal << phyId1;
    return;
  }

  // Spin until autonegotiation is complete and the link is up. TODO: timeouts
  bool negotiation_complete = false;
  bool link_up = false;
  do {
    uint16_t phy_status = bsp_eth_phy_read(kPhyAddress, Lan8742a::RegAddress::kBasicStatus);
    negotiation_complete = (phy_status & Lan8742a::BasicStatus::kAutoNegotiateCompleteBit) ? true : false;
    link_up = (phy_status & Lan8742a::BasicStatus::kLinkStatusBit) ? true : false;
  } while (!link_up || !negotiation_complete);

  // Verify we can use 100Base-TX FD
  uint16_t partner_ability = bsp_eth_phy_read(kPhyAddress, Lan8742a::RegAddress::kAutoNegotiationPartnerAbility);
  if ((partner_ability & Lan8742a::PartnerAbility::k100BaseTX_Fd) == 0U) {
    LOG_ERROR() << "100Base-TX Full Duplex is not supported.";
    return;
  }

  // 100 Mbit/s, full duplex. Receive all / promiscuous mode
  ETH->MACFFR |= ETH_MACFFR_RA | ETH_MACFFR_PM;

  // Store-and-forward for RX and TX. Receive all frames.
  ETH->DMAOMR |= ETH_DMAOMR_RSF | ETH_DMAOMR_TSF | ETH_DMAOMR_FEF | ETH_DMAOMR_FUGF;

  ETH->DMABMR |= ETH_DMABMR_PBL_32Beat;

  // Enable all interrupts
  ETH->DMAIER |= 0x0001'E7FF;

  // Enable Transmit (TODO: enable RX once DMA path is ready)
  ETH->MACCR |= ETH_MACCR_FES | ETH_MACCR_DM | ETH_MACCR_TE; // | ETH_MACCR_RE;

  NVIC_SetPriority(ETH_IRQn, 2U);
  NVIC_EnableIRQ(ETH_IRQn);
}

// USART1 DMA channels:
// DMA2 Stream 5 Channel 4: RX
// DMA2 Stream 7 Channel 4: TX
void bsp_init_uart() {
  RCC->APB2ENR = RCC_APB2ENR_USART1EN;
  RCC->APB2ENR;

  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
  RCC->AHB1ENR;

  // Memory -> USART TDR, direct mode, single bytes
  DMA2_Stream7->PAR = USART1_BASE + offsetof(USART_TypeDef, TDR);
  DMA2_Stream7->CR = DMA_SxCR_CHSEL_2 | DMA_SxCR_MINC | DMA_SxCR_DIR_0
                          | DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE;

  NVIC_SetPriority(DMA2_Stream7_IRQn, 7U);
  NVIC_EnableIRQ(DMA2_Stream7_IRQn);

  USART1->BRR = 4375; // 19.2 kbps from 84 MHz APB2 clock
  USART1->CR3 |= USART_CR3_DMAT;
  USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

RingBuffer<uint8_t, 512U> uart_tx_buffer{};
std::atomic<uint32_t> dma_current_size{};
std::atomic<uint32_t> dma_error_flags{};

bool bsp_uart_start_dma() {
  if (dma_current_size.load() != 0U) {
    return true; // DMA already active
  }

  util::Span<const uint8_t> payload_view{uart_tx_buffer.readBulk()};
  if (payload_view.size() == 0U) {
    return false; // No data to send
  }

  dma_current_size.store(payload_view.size());
  DMA2_Stream7->M0AR = reinterpret_cast<uint32_t>(payload_view.data());
  DMA2_Stream7->NDTR = payload_view.size();
  DMA2_Stream7->CR |= DMA_SxCR_EN;
  return true;
}

void bsp_uart_stop_dma() {
  DMA2->HIFCR |= DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7;

  uart_tx_buffer.discardBulk(dma_current_size.load());
  dma_current_size.store(0U);
}

extern "C" {
void DMA2_Stream7_IRQHandler() {
  const uint32_t hisr{DMA2->HISR};

  bsp_uart_stop_dma();
  bsp_uart_start_dma();

  if ((hisr & (DMA_HISR_TEIF7 | DMA_HISR_DMEIF7)) != 0) {
    dma_error_flags.store(hisr);
  }
}
}

void bsp_uart_send(util::Span<const uint8_t> payload) {
  uart_tx_buffer.enqueue(payload);

  // TODO: proper critical section
  __disable_irq();
  bsp_uart_start_dma();
  __enable_irq();
}

// 168 MHz Sysclk / AHB, 84 MHz APB2, 42 MHz APB1, from HSI 16 MHz
void bsp_init_clocks() {
  // Initialize the PLL. Src=HSI-16, M=8, N=168, P=2, Q=7.
  // Use PLLP[1:0] == 0 to select PLL_P = 2
  RCC->PLLCFGR = (7U << RCC_PLLCFGR_PLLQ_Pos) | (0U << RCC_PLLCFGR_PLLP_Pos) | (168U << RCC_PLLCFGR_PLLN_Pos) | 8U;
  RCC->CR |= RCC_CR_PLLON;

  // Set voltage scale 2
  RCC->APB1ENR = RCC_APB1ENR_PWREN;
  RCC->APB1ENR;
  PWR->CR1 = PWR_CR1_VOS_1;

  // Set peripheral bus dividers
  RCC->CFGR = RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4; // AHB not divided

  // Configure flash wait states
  FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;

  // Switch Sysclk to PLL when ready
  while ((PWR->CSR1 & PWR_CSR1_VOSRDY) == 0U) {} // wait
  while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {} // wait
  RCC->CFGR |= RCC_CFGR_SW_1; // Sysclk = PLL
}

// The address of this symbol is the base of the interrupt vector table.
extern "C" const uint32_t g_pfnVectors[];

// TODO: enable MPU and ITCM
void bsp_init_cpu() {
  // Relocate the vector table and enable fault handlers
  SCB->VTOR = reinterpret_cast<uint32_t>(g_pfnVectors);
  SCB->SHCSR = SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

  // Enable FPU (cp10, cp11), with automatic CONTROL.FPCA management
  SCB->CPACR = 0xFU << 20U;
  FPU->FPCCR = FPU_FPCCR_ASPEN_Msk;

  // Enable I-cache
  SCB->ICIALLU = 0U;
  __DSB();
  SCB->CCR = SCB_CCR_IC_Msk;
  __ISB();
}

/**
 * @brief Logging sink that writes messages synchronously to USART1.
 */
class UartLogSink final : public logging::LogSink {
public:
  bool write(util::Span<const uint8_t> message) override {
    bsp_uart_send(message);
    return true;
  }
};

class LedBlinky : public bare_metal::PeriodicTask {
public:
  LedBlinky() : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)) {}

  void tick() noexcept override  {
    GPIOC->ODR ^= GPIO_ODR_OD7_Msk;
  }
};

class UptimeLogger : public bare_metal::PeriodicTask {
  public:
  UptimeLogger() : bare_metal::PeriodicTask(std::chrono::milliseconds(2000U)) {}

  void tick() noexcept override {
    LOG_INFO() << "Uptime ms: " << hal::SchedulerClock::now().time_since_epoch().count();
  }
};

template <size_t kNumPayloadBytes>
struct EthernetPacket {
  uint8_t destination_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t source_mac[6];
  uint16_t ether_type;
  uint8_t payload[kNumPayloadBytes];
};

class EthernetTest : public bare_metal::PeriodicTask {
public:
  EthernetTest() : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)) {}
  
  void tick() noexcept override  {
    constexpr size_t kPayloadSize = 64U;

    EthernetPacket<kPayloadSize> packet{};
    packet.ether_type = kPayloadSize;
    for (size_t i = 0U; i < kPayloadSize; i++) {
      packet.payload[i] = static_cast<uint8_t>(i & 0xFFU);
    }

    const uint8_t* bytes = reinterpret_cast<uint8_t*>(&packet);
    bsp_ethernet_send(util::Span(bytes, sizeof(packet)));

    tick_count_++;
    if (tick_count_ >= 10U) {
      tick_count_ = 0U;
      LOG_INFO() << "Eth TX frames: " << ETH->MMCTGFCR;
    }
  }

private:
  uint32_t tick_count_{};
};

class ResetCause {
public:
  ResetCause() noexcept {
    value_ = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;
  }

  void log() noexcept {
    LOG_INFO() << "Reset Cause:" << ((value_ & RCC_CSR_LPWRRSTF) ? " LPWR" : "")
               << ((value_ & RCC_CSR_WWDGRSTF) ? " WWDG" : "") << ((value_ & RCC_CSR_IWDGRSTF) ? " IWDG" : "")
               << ((value_ & RCC_CSR_SFTRSTF) ? " SFT" : "") << ((value_ & RCC_CSR_PORRSTF) ? " POR" : "")
               << ((value_ & RCC_CSR_PINRSTF) ? " PIN" : "") << ((value_ & RCC_CSR_BORRSTF) ? " BOR" : "");
  }

private:
  uint32_t value_;
};

int main() {
  ResetCause reset_cause{};
  // bsp_init_watchdog();
  bsp_init_cpu();
  bsp_init_clocks();
  bsp_init_systick();
  bsp_init_gpio();
  bsp_init_uart();

  // bsp_watchdog_kick();

  UartLogSink uart_log_sink{};
  logging::setLogSink(&uart_log_sink);
  reset_cause.log();
  __enable_irq();

  bsp_init_ethernet();
  // bsp_init_pwm();

  LedBlinky led_blinky{};
  EthernetTest eth_task{};
  UptimeLogger uptime_logger{};

  led_blinky.start();
  uptime_logger.start();
  eth_task.start();

  for (;;) {
    // bsp_watchdog_kick();
    led_blinky.poll();
    uptime_logger.poll();
    eth_task.poll();
  }

  while (true) {}
}
