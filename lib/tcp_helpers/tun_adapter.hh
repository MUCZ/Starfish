#ifndef TUN_ADAPTER
#define TUN_ADAPTER

#include <optional>
#include <unordered_map>
#include <utility>
#include "../util/tun.hh"
#include "../util/socket.hh"
#include "tcp_config.hh"
#include "tcp_header.hh"
#include "tcp_segment.hh"
#include "ipv4_datagram.hh"
#include "file_descriptor.hh"

class FdAdapterBase {
  private:
    FdAdapterConfig _cfg{};  //!< Configuration values
    bool _listen = false;    //!< Is the connected TCP FSM in listen state?

  protected:
    FdAdapterConfig &config_mutable() { return _cfg; }

  public:
    //! \brief Set the listening flag
    //! \param[in] l is the new value for the flag
    void set_listening(const bool l) { _listen = l; }

    //! \brief Get the listening flag
    //! \returns whether the FdAdapter is listening for a new connection
    bool listening() const { return _listen; }

    //! \brief Get the current configuration
    //! \returns a const reference
    const FdAdapterConfig &config() const { return _cfg; }

    //! \brief Get the current configuration (mutable)
    //! \returns a mutable reference
    FdAdapterConfig &config_mut() { return _cfg; }

    //! Called periodically when time elapses
    void tick(const size_t) {}
};

class TCPOverIPv4Adapter : public FdAdapterBase {
  public:
    std::optional<TCPSegment> unwrap_tcp_in_ip(const InternetDatagram &ip_dgram);

    InternetDatagram wrap_tcp_in_ip(TCPSegment &seg);
};

class TCPOverIPv4OverTunFdAdapter : public TCPOverIPv4Adapter {
  private:
    TunFD _tun;

  public:
    //! Construct from a TunFD
    explicit TCPOverIPv4OverTunFdAdapter(TunFD &&tun) : _tun(std::move(tun)) {}

    //! Attempts to read and parse an IPv4 datagram containing a TCP segment related to the current connection
    std::optional<TCPSegment> read() {
        InternetDatagram ip_dgram;
        if (ip_dgram.parse(_tun.read()) != ParseResult::NoError) {
            return {};
        }
        return unwrap_tcp_in_ip(ip_dgram);
    }

    //! Creates an IPv4 datagram from a TCP segment and writes it to the TUN device
    void write(TCPSegment &seg) { _tun.write(wrap_tcp_in_ip(seg).serialize()); }

    //! Access the underlying TUN device
    operator TunFD &() { return _tun; }

    //! Access the underlying TUN device
    operator const TunFD &() const { return _tun; }
};
#endif /* TUN_ADAPTER */
