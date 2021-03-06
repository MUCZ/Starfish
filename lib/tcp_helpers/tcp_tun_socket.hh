#ifndef TCP_TUN_SOCKET
#define TCP_TUN_SOCKET

#include "byte_stream.hh"
#include "eventloop.hh"
#include "file_descriptor.hh"
#include "tcp_config.hh"
#include "tcp_connection.hh"
#include "tun_adapter.hh"

#include <atomic>
#include <cstdint>
#include <optional>
#include <thread>
#include <vector>

//! Multithreaded wrapper around TCPConnection that approximates the Unix sockets API
class TUNSocket : public LocalStreamSocket {
  private:
    //! Stream socket for reads and writes between owner and TCP thread
    LocalStreamSocket _thread_data;

    //! Adapter to underlying datagram socket (e.g., UDP or IP)
    TCPOverIPv4OverTunFdAdapter _datagram_adapter;

    //! Set up the TCPConnection and the event loop
    void _initialize_TCP(const TCPConfig &config);

    //! TCP state machine
    std::optional<TCPConnection> _tcp{};

    //! eventloop that handles all the events (new inbound datagram, new outbound bytes, new inbound bytes)
    EventLoop _eventloop{};

    //! Process events while specified condition is true
    void _tcp_loop(const std::function<bool()> &condition);

    //! Main loop of TCPConnection thread
    void _tcp_main();

    //! Handle to the TCPConnection thread; owner thread calls join() in the destructor
    std::thread _tcp_thread{};

    //! Construct LocalStreamSocket fds from socket pair, initialize eventloop
    TUNSocket(std::pair<FileDescriptor, FileDescriptor> data_socket_pair, TCPOverIPv4OverTunFdAdapter &&datagram_interface);

    std::atomic_bool _abort{false};  //!< Flag used by the owner to force the TCPConnection thread to shut down

    bool _inbound_shutdown{false};  //!< Has TUNSocket shut down the incoming data to the owner?

    bool _outbound_shutdown{false};  //!< Has the owner shut down the outbound data to the TCP connection?

    bool _fully_acked{false};  //!< Has the outbound data been fully acknowledged by the peer?

  public:
    //! Construct from the interface that the TCPConnection thread will use to read and write datagrams
    //! Using the default name for tun device
    explicit TUNSocket(TCPOverIPv4OverTunFdAdapter &&datagram_interface = TCPOverIPv4OverTunFdAdapter(TunFD("starfish_tun")));
    //! Using user-defined name, NOTE: make sure the tun device is available and the related routing rules are configured.
    explicit TUNSocket(std::string name);


    //! Close socket, and wait for TCPConnection to finish
    //! \note Calling this function is only advisable if the socket has reached EOF,
    //! or else may wait foreever for remote peer to close the TCP connection.
    void wait_until_closed();

    //! Connect using the specified configurations; blocks until connect succeeds or fails
    void connect(const TCPConfig &c_tcp, const FdAdapterConfig &c_ad);
    //! Connect using default source ip
    void connect(const Address &address);
    //! Connect using assigned source_ip, NOTE: route rule for related source ip should be configured using `iptables`
    void connect(const std::string source_ip, const Address &address);


    //! Listen and accept using the specified configurations; blocks until accept succeeds or fails
    void listen_and_accept(const TCPConfig &c_tcp, const FdAdapterConfig &c_ad);

    bool in_bound_shutdown() const {return _inbound_shutdown;}
    bool out_bound_shutdown() const { return _outbound_shutdown; }
    //! When a connected socket is destructed, it will send a RST
    ~TUNSocket();

    //! \name
    //! This object cannot be safely moved or copied, since it is in use by two threads simultaneously

    //!@{
    TUNSocket(const TUNSocket &) = delete;
    TUNSocket(TUNSocket &&) = delete;
    TUNSocket &operator=(const TUNSocket &) = delete;
    TUNSocket &operator=(TUNSocket &&) = delete;
    //!@}

    //! \name
    //! Some methods of the parent Socket wouldn't work as expected on the TCP socket, so delete them

    //!@{
    void bind(const Address &address) = delete;
    Address local_address() const = delete;
    Address peer_address() const = delete;
    void set_reuseaddr() = delete;
    //!@}
};

//! \class TUNSocket
//! This class involves the simultaneous operation of two threads.
//!
//! One, the "owner" or foreground thread, interacts with this class in much the
//! same way as one would interact with a TCPSocket: it connects or listens, writes to
//! and reads from a reliable data stream, etc. Only the owner thread calls public
//! methods of this class.
//!
//! The other, the "TCPConnection" thread, takes care of the back-end tasks that the kernel would
//! perform for a TCPSocket: reading and parsing datagrams from the wire, filtering out
//! segments unrelated to the connection, etc.
//!
//! There are a few notable differences between the TUNSocket and TCPSocket interfaces:
//!
//! - a TUNSocket can only accept a single connection
//! - listen_and_accept() is a blocking function call that acts as both [listen(2)](\ref man2::listen)
//!   and [accept(2)](\ref man2::accept)
//! - if TUNSocket is destructed while a TCP connection is open, the connection is
//!   immediately terminated with a RST (call `wait_until_closed` to avoid this)




#endif /* TCP_TUN_SOCKET */
