#pragma once
/*
 * @file  ethernet.h
 * @brief STM32F746 Ethernet MAC and LAN8742A PHY driver.
 */

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "util/span.h"

/**
 * @brief Owns the Ethernet transmit DMA ring and controls the MAC and PHY.
 *
 * Only one initialized instance is supported because the MCU has one Ethernet
 * peripheral and one associated interrupt vector.
 */
class Ethernet {
 public:
  Ethernet() noexcept = default;

  Ethernet(const Ethernet &) = delete;
  Ethernet &operator=(const Ethernet &) = delete;
  Ethernet(Ethernet &&) = delete;
  Ethernet &operator=(Ethernet &&) = delete;

  /**
   * @brief Initializes the Ethernet MAC, transmit DMA ring, PHY, and interrupt.
   *
   * This function blocks until PHY auto-negotiation completes.
   */
  void init() noexcept;

  /**
   * @brief Copies and queues one Ethernet frame for transmission.
   * @param[in] payload Frame bytes. Data beyond the maximum frame size is
   * truncated.
   * @return true when the frame was queued; false when no descriptor was
   * available.
   * @pre init() completed successfully.
   */
  bool send(util::Span<const uint8_t> payload) noexcept;

  /** @brief Dispatches the hardware interrupt to the initialized instance. */
  static void handleInterrupt() noexcept;

 private:
  struct Lan8742a {
    struct RegAddress {
      static constexpr uint16_t kBasicControl{0U};
      static constexpr uint16_t kBasicStatus{1U};
      static constexpr uint16_t kPhyId1{2U};
      static constexpr uint16_t kPhyId2{3U};
      static constexpr uint16_t kAutoNegotiationAdvertise{4U};
      static constexpr uint16_t kAutoNegotiationPartnerAbility{5U};
      // Other registers omitted for now.
    };

    struct BasicStatus {
      static constexpr uint16_t kExtendedCapabilities{1U << 0U};
      static constexpr uint16_t kJabberDetectBit{1U << 1U};
      static constexpr uint16_t kLinkStatusBit{1U << 2U};
      static constexpr uint16_t kRemoteFaultBit{1U << 4U};
      static constexpr uint16_t kAutoNegotiateCompleteBit{1U << 5U};
      static constexpr uint16_t kExtendedStatus{1U << 8U};
      // Other bits omitted for now.
    };

    struct PartnerAbility {
      static constexpr uint16_t k10BaseT{1U << 5U};
      static constexpr uint16_t k10BaseTFullDuplex{1U << 6U};
      static constexpr uint16_t k100BaseTx{1U << 7U};
      static constexpr uint16_t k100BaseTxFullDuplex{1U << 8U};
      // Other bits omitted for now.
    };

    static constexpr uint16_t kExpectedPhyId1{0x0007U};
    static constexpr uint16_t kExpectedPhyId2{0xC130U};
    // The low four PHY ID bits contain the revision and are intentionally
    // ignored.
    static constexpr uint16_t kExpectedPhyId2Mask{0xFFF0U};
  };

  struct EthDmaTxDescriptor {
    static constexpr uint32_t kStatusOwn{1UL << 31U};
    static constexpr uint32_t kInterruptOnComplete{1UL << 30U};
    static constexpr uint32_t kLastSegment{1UL << 29U};
    static constexpr uint32_t kFirstSegment{1UL << 28U};
    static constexpr uint32_t kDisableCrc{1UL << 27U};
    static constexpr uint32_t kDisablePad{1UL << 26U};
    static constexpr uint32_t kTxTimestampEnable{1UL << 25U};
    static constexpr uint32_t kChecksumInsertion0{1UL << 24U};
    static constexpr uint32_t kChecksumInsertion1{1UL << 23U};
    static constexpr uint32_t kTxEndOfRing{1UL << 21U};
    static constexpr uint32_t kSecondAddressChained{1UL << 20U};
    static constexpr uint32_t kTxTimestampStatus{1UL << 17U};
    static constexpr uint32_t kIpHeaderError{1UL << 16U};
    static constexpr uint32_t kErrorSummary{1UL << 15U};
    static constexpr uint32_t kJabberTimeout{1UL << 14U};
    static constexpr uint32_t kFrameFlushed{1UL << 13U};
    static constexpr uint32_t kIpPayloadError{1UL << 12U};
    static constexpr uint32_t kLossOfCarrier{1UL << 11U};
    static constexpr uint32_t kNoCarrier{1UL << 10U};
    static constexpr uint32_t kLateCollision{1UL << 9U};
    static constexpr uint32_t kExcessiveCollision{1UL << 8U};
    static constexpr uint32_t kVlanFrame{1UL << 7U};
    static constexpr uint32_t kCollisionCountMask{0x78U};
    static constexpr uint8_t kCollisionCountOffset{3U};
    static constexpr uint32_t kExcessiveDeferral{1UL << 2U};
    static constexpr uint32_t kUnderflowError{1UL << 1U};
    static constexpr uint32_t kDeferredTransmit{1UL << 0U};

    void setBufferSizes(uint16_t buffer_size_1, uint16_t buffer_size_2) noexcept;
    bool isOwnedByDma() const noexcept;

    uint32_t status_control{};
    uint32_t byte_counts{};
    uint8_t *buffer_ptr{};
    EthDmaTxDescriptor *next_ptr{};
  };

  static constexpr uint8_t kPhyAddress{0U};
  static constexpr size_t kNumTxBuffers{8U};
  static constexpr size_t kMaxEthernetFrameSize{1518U};
  static constexpr uint32_t kTxDescriptorFlags{
      EthDmaTxDescriptor::kInterruptOnComplete | EthDmaTxDescriptor::kLastSegment |
      EthDmaTxDescriptor::kFirstSegment | EthDmaTxDescriptor::kSecondAddressChained};

  uint16_t phyTransaction(uint8_t phy_address, uint8_t mii_register, uint16_t data,
                          bool is_write) noexcept;
  uint16_t phyRead(uint8_t phy_address, uint8_t mii_register) noexcept;
  void phyWrite(uint8_t phy_address, uint8_t mii_register, uint16_t data) noexcept;
  EthDmaTxDescriptor *allocateTxDescriptor() noexcept;
  void handleInterruptInstance() noexcept;

  static Ethernet *instance_;

  // DMA descriptors require word alignment.
  alignas(4) std::array<EthDmaTxDescriptor, kNumTxBuffers> tx_descriptors_{};

  // Buffers use cache-line alignment in preparation for enabling the data cache.
  alignas(32) std::array<std::array<uint8_t, kMaxEthernetFrameSize>, kNumTxBuffers> tx_buffers_{};

  std::atomic<uint32_t> free_index_{};
  std::atomic<uint32_t> pending_index_{};
};