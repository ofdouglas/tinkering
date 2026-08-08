#include "bootloader/protocol.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

using bootloader::AppVersion1;
using bootloader::BoardVersion1;
using bootloader::BootloaderInfo1;
using bootloader::CommandType;
using bootloader::DataIdentifier;
using bootloader::ErrorCode;
using bootloader::ImageStatus;
using bootloader::ImageStatus1;
using bootloader::isValidCommandType;
using bootloader::isValidDataIdentifier;
using bootloader::isValidErrorCode;
using bootloader::kProtocolVersion;

PYBIND11_MODULE(protocol, m) {
    m.attr("kProtocolVersion") = kProtocolVersion;

    m.def("isValidCommandType", &isValidCommandType, "Check if a CommandType is valid");
    m.def("isValidDataIdentifier", &isValidDataIdentifier, "Check if a DataIdentifier is valid");
    m.def("isValidErrorCode", &isValidErrorCode, "Check if an ErrorCode is valid");

    py::enum_<ImageStatus>(m, "ImageStatus")
        .value("kUnknown", ImageStatus::kUnknown)
        .value("kNotSupported", ImageStatus::kNotSupported)
        .value("kInvalid", ImageStatus::kInvalid)
        .value("kErased", ImageStatus::kErased)
        .value("kBusy", ImageStatus::kBusy)
        .value("kValid", ImageStatus::kValid)
        .export_values();

    py::enum_<DataIdentifier>(m, "DataIdentifier")
        .value("kUnknown", DataIdentifier::kUnknown)
        .value("kBootloaderInfo1", DataIdentifier::kBootloaderInfo1)
        .value("kImageStatus1", DataIdentifier::kImageStatus1)
        .value("kBoardVersion1", DataIdentifier::kBoardVersion1)
        .value("kAppVersion1", DataIdentifier::kAppVersion1)
        .export_values();

    py::enum_<ErrorCode>(m, "ErrorCode")
        .value("kUnknown", ErrorCode::kUnknown)
        .value("kInvalidDataIdentifier", ErrorCode::kInvalidDataIdentifier)
        .value("kWriteOnlyDataIdentifier", ErrorCode::kWriteOnlyDataIdentifier)
        .value("kInvalidMemAddress", ErrorCode::kInvalidMemAddress)
        .value("kInvalidMemSize", ErrorCode::kInvalidMemSize)
        .value("kWriteFailed", ErrorCode::kWriteFailed)
        .value("kTransferTimeout", ErrorCode::kTransferTimeout)
        .value("kNotPrepared", ErrorCode::kNotPrepared)
        .value("kInvalidImageFormat", ErrorCode::kInvalidImageFormat)
        .value("kInvalidImageHeaderVersion", ErrorCode::kInvalidImageHeaderVersion)
        .value("kInvalidBoardType", ErrorCode::kInvalidBoardType)
        .value("kInvalidBoardVersion", ErrorCode::kInvalidBoardVersion)
        .value("kInvalidAppVersion", ErrorCode::kInvalidAppVersion)
        .value("kInvalidImageCrc", ErrorCode::kInvalidImageCrc)
        .value("kInvalidHeaderCrc", ErrorCode::kInvalidHeaderCrc)
        .export_values();

    py::enum_<CommandType>(m, "CommandType")
        .value("kUnknown", CommandType::kUnknown)
        .value("kReadDataIdentifier", CommandType::kReadDataIdentifier)
        .value("kWriteDataIdentifier", CommandType::kWriteDataIdentifier)
        .value("kReset", CommandType::kReset)
        .value("kBootApplication", CommandType::kBootApplication)
        .value("kCommandSuccess", CommandType::kCommandSuccess)
        .value("kCommandPending", CommandType::kCommandPending)
        .value("kCommandFailed", CommandType::kCommandFailed)
        .value("kSegmentAck", CommandType::kSegmentAck)
        .value("kSegmentNak", CommandType::kSegmentNak)
        .value("kPrepareErase", CommandType::kPrepareErase)
        .value("kStartErase", CommandType::kStartErase)
        .value("kPrepareAppDownload", CommandType::kPrepareAppDownload)
        .value("kStartAppDownload", CommandType::kStartAppDownload)
        .value("kEndAppDownload", CommandType::kEndAppDownload)
        .value("kSegmentTransfer", CommandType::kSegmentTransfer)
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
