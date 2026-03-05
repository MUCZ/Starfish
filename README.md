# Starfish

## An Eventloop-Based User-Level TCP Stack

# Overview

* Implemented in C++17, Starfish is a foundational user-space TCP socket library based on an **event loop** model.
* Uses Linux/TUN devices to send and receive IP packets directly, constructing TCP headers in user space, **allowing the local machine to send TCP packets with arbitrary IP addresses**.
* Implements core TCP features: automatic segmentation and reassembly, reliable transmission via cumulative acknowledgments and timeout retransmissions, and flow control using sliding windows.
* Underlying buffers use smart pointer-based zero-copy design and scatter-gather I/O to minimize redundant copies, enabling high performance.

<p align="center">
  <img src="https://github.com/MUCZ/Starfish/blob/main/img/tunsocket.png">
</p>

# Use Cases

* **Learning purposes**: TCP protocol, event loop model, network programming, virtual networks, iptables, tunneling proxies, and modern C++ concepts.
* Using the TUNSocket abstraction, it can **implement VPN protocols like OpenVPN and IP-layer tunneling proxies** (unlike SOCKS or Shadowsocks, which operate at the TCP/UDP layer).

  * **OpenVPN client**: Routes all traffic through a TUN device; processes data at the application layer (e.g., encryption), then re-encapsulates and sends it via a physical NIC.
  * **OpenVPN proxy server**: Receives traffic from the physical NIC, processes it at the application layer (e.g., decryption), and re-encapsulates it via the TUN device for internal routing, creating a virtual encrypted network.
  * **End server**: Unaware of the proxy; sees the client's real IP. In contrast, with SOCKS proxies, the server only sees the proxy IP, making it impossible to distinguish multiple clients behind the same proxy.

<p align="center">
  <img src="https://github.com/MUCZ/Starfish/blob/main/img/tunsocket_vpn.png">
</p>

* Implements a “dummy TCP over IP,” which can be adapted to **user-defined protocols over UDP**, compatible with existing infrastructure while supporting custom reliable transport protocols (similar to Google’s QUIC).
* Can be used to **build test programs**, e.g., constructing TCP handshake packets without allocating buffers, simulating load or attack scenarios at low cost.

# Examples

* **/apps/netcat**: Netcat service based on TUNSocket, communicates with a real nc client.
* **/apps/chargen_tun**: TUNSocket-based chargen service; initiates a connection and sends arbitrary characters, then reports bandwidth to evaluate performance.
* **/apps/chargen**: Same as above but uses native sockets for performance comparison.
* **/apps/webget**: Simple HTTP client based on TUNSocket, sends GET requests to a server.
* **/apps/tcp_benchmark**: Tests the performance of the TCP implementation.

# Directory Structure

* **/apps**: Example applications.
* **/etc**: CMake macros, C++ compile options, and tun.sh device IP configuration.
* **/tests**: Test fixtures and unit tests.
* **/tun.sh**: Script to start/stop the TUN device, configure port forwarding and NAT.
* **/lib**

  * **/**: TCP protocol implementation.
  * **/tcp_helpers**: TCP and IP data formats and their encoders/decoders, TUNSocket implementation.
  * **/util**: Wrappers for address, buffer, file descriptors, parser, socket, tun, syscalls, runtime errors, etc.

# Performance

* Comparing chargen, chargen_tun, and netcat file transfer shows that **user-space TUNSocket achieves performance close to native sockets**, limited mainly by kernel-to-user copy overhead and virtual NIC routing cost.
* Using Linux/TUN introduces two additional copies between kernel and user space, meaning the theoretical maximum throughput is roughly half of native sockets.
* Detailed performance metrics and optimization strategies are documented in this Zhihu column: [Starfish TCP Stack Performance](https://zhuanlan.zhihu.com/p/414279516)

# Using TUNSocket

* Creating a TUNSocket with default or custom device name:

  ```cpp
  TUNSocket tunsock0;       // Default device
  TUNSocket tunsock1("tun1"); // Custom device
  ```

* Specifying the source IP for outgoing TCP packets:

  ```cpp
  TUNSocket tunsock1("tun1");
  // tunsock1.connect(dest_ip); // default source IP
  tunsock1.connect("169.254.144.2", destination_ip); // user-defined source IP
  ```

* Setting TCP retransmission timeout:

  ```cpp
  TUNSocket tunsock;
  TCPConfig tcp_config;
  tcp_config.rt_timeout = 100;
  tunsock.connect(tcp_config, destination_ip);
  ```

* Configuring buffer capacity:

  ```cpp
  TUNSocket tunsock;
  TCPConfig tcp_config;
  tcp_config.recv_capacity = 64000;
  tunsock.connect(tcp_config, destination_ip);
  ```

# Running Examples

Compile the code:

```bash
mkdir build
cd build
make -j8
```

Start the TUN device and routing:

```bash
sudo ../tun.sh start
```

* **Chargen service**: After sending completes, displays bandwidth.
  On host B:

  ```bash
  sudo nc -l <port> >/dev/null
  ```

  On host A:

  ```bash
  ./apps/chargen.cc <B_ip> <port>
  ./apps/chargen_tun.cc <B_ip> <port>
  ```

* **Webget**:
  Compile simple_http_server on host B:

  ```bash
  cd /simple_http_server
  go build simple_http_server
  ```

  Run server:

  ```bash
  sudo ./simple_http_server
  ```

  On host A:

  ```bash
  ./apps/webget <B_ip> /hello
  ./apps/webget <B_ip> /bye
  ```

* **Netcat**:
  On host B, listen on a port:

  ```bash
  nc -l 8080
  ./netcat -l 8080
  ```

  On host A, connect:

  ```bash
  nc <B_ip> 8080
  ./netcat <B_ip> 8080
  ```

  Once connected, terminal input is transmitted to the other host.
