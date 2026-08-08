#pragma once
/*
 * @file  network_management.h
 * @brief Network management service for the custom link-layer protocol.
 */

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>
#include <algorithm>

/*
 * @brief Network management service for the custom link-layer protocol.
 *
 * @todo Design for this:
 *
 * It should be a simple service which provides:
 *  - Protocol-level error reporting of things like: 
 *      - unsupported service type,
 *      - unsupported header extensions,
 *      - frame size too large,
 *      - frame checksum error (TBD reporting policy for this)
 *      - other protocol-level errors as needed
 *      - frame size rejected by the service (service returned an error to the protocol stack)
 *      - TBD: any other reason the frame isn't successfully delivered to the service
 *
 *  - Advertising of capabilities:
 *      - supported service types,     (may be unnecessary: you can just try to send a frame and see if you get an unsupported service type error?)
 *                                     (but it could be useful for debugging, and ruling out "service integration error" -- makes intent clear)
 *      - supported header extensions,
 *      - supported maximum frame size
 *
 * Potential features:
 *  - Monitor link state to report to the application if the link is up or not:
 *    - Link state can be polled by the application, or can be configured to be reported via a callback.
 *    - The link is up if any Service has received a valid frame in the last X seconds,
 *      which includes NetworkManagement frames (in particular, Error messages).
 *    - This link state provides a single source of truth for the application on:
 *      - State of the Peer:
 *        - Disconnected  (no valid frames received in the last X seconds)
 *        - Connected     (any valid frames received in the last X seconds, including NetworkManagement frames)
 *        - LinkError     (Connected, with any link-level compatibility errors reported by the peer)
 *        - Healthy       (Connected, with no link-level compatability errors reported by the peer)
 *    - This feature is intended to make it easy to diagnose application-level connectivity issues.
 *      For example if no user services are working, the state allows immediate triage into these buckets:
 *        - Disconnected:  Physical link is down or peer is down.
 *        - Connected:     Most likely the issue is in the user services. It could also be an integration issue
 *                         (like Service not receiving callbacks because they weren't registered correctly).
 *        - LinkError:     Unsupported service type or protocol extension.
 * 
 *  - Standardized basic metrics reporting (optional):
 *     - MCU applications can choose to have the NM service periodically send metrics packets to the host for logging.
 *     - This should be simple, optional, and not consume much bandwidth.
 *     - The feature should cover just a few key metrics. Some ideas:
 *       - TX frame count
 *       - RX frame count  (good, bad)
 *       - RX error counts (bad CRC, bad frame size, unsupported service type, unsupported protocol extension, etc.)
 *       - Receiver buffer high water mark and/or overrun count (of the UART RX buffer)
 *
 * Possible more advanced (optional) features:
 *  - Configuration of protocol extensions
 *  - Configuration of flow control
 *  - Configuration of baud rate (common base + negotiation. TBD if worthwhile.)
 */
namespace hdlc::network_management {

// Errors should have a type and a value, so we can report things like {kInvalidServiceType, value=0x12}
// Consider if we can re-use the same packets / fields for other information. Ex: {kSupportedExtensions, value=0x03}
struct ErrorMessage { /* TODO */ };

struct Capabilities { /* TODO */ };

} // namespace hdlc::network_management