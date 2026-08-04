#include "bootloader/image_header.h"
#include "bootloader/mcu/bootloader.h"
#include "bootloader/test/mocks.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

#if defined(__unix__)
#include <sys/mman.h>
#endif

using Bootloader::BoardType;
using Bootloader::HeaderStart;
using Bootloader::ImageHeader_Application_v1;
using Bootloader::ImageType;

namespace {

constexpr uint32_t kAppFlashBase = 0x08040000U;

ImageHeader_Application_v1 makeValidApplicationHeader() {
    ImageHeader_Application_v1 header{};
    header.header_start.magic = HeaderStart::kMagic;
    header.header_start.image_type = static_cast<uint8_t>(ImageType::kApplication);
    header.header_start.header_version = 1U;
    header.header_start.header_size_bytes =
        static_cast<uint16_t>(sizeof(ImageHeader_Application_v1) - sizeof(HeaderStart));
    header.board_type = static_cast<uint16_t>(BoardType::kStm32F7Discovery);
    header.board_version_major = 1U;
    header.board_version_minor = 0U;
    header.app_version_major = 1U;
    header.app_version_minor = 0U;
    header.image_size_bytes = 1024U;
    header.image_lma_address = kAppFlashBase;
    header.image_entry_address = kAppFlashBase + 0x100U;
    header.image_crc = 0U;
    header.header_crc = 0U;
    return header;
}

} // namespace

TEST(BootloaderMcu, ValidateApplicationAcceptsValidHeader) {
#if !defined(__unix__)
    GTEST_SKIP() << "Requires mmap MAP_FIXED (Unix)";
#endif

    const ImageHeader_Application_v1 header = makeValidApplicationHeader();
    const size_t map_size = sizeof(ImageHeader_Application_v1);

    void* mapped = mmap(reinterpret_cast<void*>(static_cast<uintptr_t>(kAppFlashBase)),
        map_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
        -1,
        0);
    ASSERT_NE(mapped, MAP_FAILED) << "mmap at app flash base failed";
    memcpy(mapped, &header, map_size);

    Bootloader::test::MockTransport transport;
    Bootloader::test::MockReset reset;

    const Memory::Region region{
        kAppFlashBase,
        static_cast<uint32_t>(map_size),
        static_cast<uint32_t>(Memory::Region::Attributes::kReadable) |
            static_cast<uint32_t>(Memory::Region::Attributes::kBootable),
        nullptr,
    };

    std::array<Memory::Region, 1U> regions{region};
    Bootloader::Bootloader bootloader{
        Span<const Memory::Region>(regions.data(), regions.size()),
        transport,
        reset,
    };

    EXPECT_TRUE(bootloader.validateApplication());

    munmap(mapped, map_size);
}
