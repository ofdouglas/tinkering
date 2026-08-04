#include "bootloader/protocol.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

using Bootloader::AppVersion1;
using Bootloader::BoardVersion1;
using Bootloader::BootloaderInfo1;
using Bootloader::ImageStatus;
using Bootloader::ImageStatus1;
using Bootloader::MessageType;
using Bootloader::TransferType;
using Bootloader::isValidMessageType;

PYBIND11_MODULE(protocol, m) {
    m.def("isValidMessageType", &isValidMessageType, "Check if a MessageType is valid");

    py::enum_<ImageStatus>(m, "ImageStatus")
        .value("kUnknown", ImageStatus::kUnknown)
        .value("kNotSupported", ImageStatus::kNotSupported)
        .value("kInvalid", ImageStatus::kInvalid)
        .value("kErased", ImageStatus::kErased)
        .value("kBusy", ImageStatus::kBusy)
        .value("kValid", ImageStatus::kValid)
        .export_values();

    py::enum_<TransferType>(m, "TransferType")
        .value("kUnknown", TransferType::kUnknown)
        .value("kFlashWrite", TransferType::kFlashWrite)
        .value("kFlashRead", TransferType::kFlashRead)
        .value("kSramWrite", TransferType::kSramWrite)
        .value("kSramRead", TransferType::kSramRead)
        .export_values();

    py::enum_<MessageType>(m, "MessageType")
        .value("kUnknown", MessageType::kUnknown)
        .value("kCommandAccept", MessageType::kCommandAccept)
        .value("kCommandReject", MessageType::kCommandReject)
        .value("kSegmentAck", MessageType::kSegmentAck)
        .value("kSegmentNak", MessageType::kSegmentNak)
        .value("kTransferSuccess", MessageType::kTransferSuccess)
        .value("kTransferFailed", MessageType::kTransferFailed)
        .value("kBootloaderInfo1", MessageType::kBootloaderInfo1)
        .value("kImageStatus1", MessageType::kImageStatus1)
        .value("kBoardVersion1", MessageType::kBoardVersion1)
        .value("kAppVersion1", MessageType::kAppVersion1)
        .value("kSetStartAddress", MessageType::kSetStartAddress)
        .value("kSetSizeBytes", MessageType::kSetSizeBytes)
        .value("kDoErase", MessageType::kDoErase)
        .value("kStartTransfer", MessageType::kStartTransfer)
        .value("kMemTransferSegment", MessageType::kMemTransferSegment)
        .value("kFinalizeTransfer", MessageType::kFinalizeTransfer)
        .value("kReset", MessageType::kReset)
        .export_values();

    py::class_<BootloaderInfo1>(m, "BootloaderInfo1")
        .def(py::init<>())
        .def(py::init<uint8_t, uint8_t, uint8_t>())
        .def_readwrite("magic", &BootloaderInfo1::magic)
        .def_readwrite("protocol_version", &BootloaderInfo1::protocol_version)
        .def_readwrite("bootloader_major", &BootloaderInfo1::bootloader_major)
        .def_readwrite("bootloader_minor", &BootloaderInfo1::bootloader_minor)
        .def_readonly_static("kMagic", &BootloaderInfo1::kMagic);

    py::class_<ImageStatus1>(m, "ImageStatus1")
        .def(py::init<>())
        .def(py::init<uint8_t, uint8_t>())
        .def_readwrite("status_a", &ImageStatus1::status_a)
        .def_readwrite("status_b", &ImageStatus1::status_b);

    py::class_<BoardVersion1>(m, "BoardVersion1")
        .def(py::init<>())
        .def(py::init<uint16_t, uint8_t, uint8_t>())
        .def_readwrite("board_type", &BoardVersion1::board_type)
        .def_readwrite("board_version_major", &BoardVersion1::board_version_major)
        .def_readwrite("board_version_minor", &BoardVersion1::board_version_minor);

    py::class_<AppVersion1>(m, "AppVersion1")
        .def(py::init<>())
        .def(py::init<uint8_t, uint8_t>())
        .def_readwrite("app_version_major", &AppVersion1::app_version_major)
        .def_readwrite("app_version_minor", &AppVersion1::app_version_minor);
}
