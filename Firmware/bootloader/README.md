# 32-bit MCU Bootloader README

/* Transport-agnostic Bootloader for 32-bit MCUs
    - Supports image transfers of [1, 2^24) 32-bit words (64 MB max)
    - Transfer to / from arbitrary addresses
    - All messages fit in 8 bytes except transfer segments, which are (N) 32-bit data words + 1 32-bit command word
    - Data identifiers with 16-bit address and 32-bit value
    - All packets start with {reserved:2, command_type:6}
    - Little endian encoding

    TODO features list:
    - Support data file transfers (upload and download)
    - Support multiple application images (A and B)
*/

// There are three command formats:
// 1. General Command Format   (8 bytes)
// 2. Memory Command Format    (8 bytes)
// 3. Segment Transfer Format  (N+1 32-bit words)


/* Example Sequences:

   Data Identifier Sequence:
   Client -> Bootloader:                Bootloader -> Client:
    - kReadDataIdentifier (id)          - kReadDataIdentifier (id, value)
    - kWriteDataIdentifier (id, value)  - kWriteDataIdentifier (id, value)
    ------------------------------------------------------------
    - kReadDataIdentifier (id)          - kCommandFailed (cmdType=kReadDataIdentifier, errorCode=kInvalidDataIdentifier)
    - kWriteDataIdentifier (id, value)  - kCommandFailed (cmdType=kWriteDataIdentifier, errorCode=kWriteOnlyDataIdentifier)

   App Erase and Download Sequence:
   Client -> Bootloader:            Bootloader -> Client:
    - kPrepareErase                     - kCommandSuccess
    - kStartErase                       - kCommandPending ...
    - ...                               - kCommandSuccess
    ------------------------------------------------------------
    - kPrepareAppDownload               - kCommandSuccess
    - kStartAppDownload                 - kCommandSuccess
    - kSegmentTransfer                  - kSegmentAck ...
    - ...                               - kSegmentNak ...  // TODO: handle retries
    - kEndAppDownload                   - kCommandPending ... (Bootloader verifies image)
    - ...                               - kCommandSuccess  or kCommandFailed (errorCode=kInvalidImageCrc or kInvalidHeaderCrc)
    ------------------------------------------------------------
    - kBootApplication                  - kCommandSuccess

   App Erase and Download Error Sequences:
   Client -> Bootloader:            Bootloader -> Client:
    - kPrepareErase                     - kCommandFailed (cmdType=kPrepareErase, errorCode=kInvalidMemAddress)
    - kStartAppDownload                 - kCommandFailed (cmdType=kStartAppDownload, errorCode=kNotPrepared)
    - kEndAppDownload                   - kCommandFailed (cmdType=kEndAppDownload, errorCode=kInvalidImageCrc)
    - kSegmentTransfer                  - kCommandFailed (cmdType=kSegmentTransfer, errorCode=kWriteFailed)
*/
