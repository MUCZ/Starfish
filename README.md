# Starfish
An Eventloop-based user-level TCP stack.

# 介绍

- 基于事件循环模式的用户态套接字网络编程基础库
- 基于Linux/tun设备直接收发ip数据报，在用户态构建TPC Header
- 实现了TCP协议的主要两大部分: 基于累计确认和超时重传的可靠传输和基于滑动窗口的流量控制 
- **用途**
  - 学习用途，帮助理解TCP协议\事件循环模式\网络编程\虚拟网络\iptables\隧道代理\现代C++相关
  - 利用TUNSocket的封装，可以实现OpenVPN等类似协议,实现IP层级别的隧道代理工具(与socks,shadowsocks等*socks协议的不同，在于后者是TCP\UDP级别的代理)
    - OpenVPN客户端：将所有流量路由至TUN设备，在应用层处理数据（例如加密）后重新封装流量，经物理网卡发送至代理
    - OpenVPN代理端：由物理网卡接受流量，在应用层处理（例如解密）后重新封装经TUN设备路由至内网,实现虚拟网络加密代理访问
    - 服务端：感知不到代理的存在，认为接受到的客户的IP地址即客户的真实地址 (在*socks协议代理的情况下，服务端收到的ip是代理的ip，因此无法区分同一个代理服务器的多个客户的IP)
  - 本仓库实现了"my dummy TCP over IP",可用类似的模式实现'"user-defined protocol" over UDP', 与现有网络基础设施兼容的同时使用用户自定义的可靠传输协议，谷歌的QUIC即是这种模式
  - 用于构建测试程序，例如构建三次握手的TCP数据包而不用分配缓冲区，低成本地模拟压力负载，恶意攻击

# 例程 

- /apps/netcat : 基于TUNSocket的netcat服务，支持与其他nc客户端通信
- /apps/chargen_tun : 基于TUNSocket的chargen服务，主动发起连接后向另一端发送任意字符，发送完成后输出带宽，用于衡量性能
- /apps/chargen : 同上，但使用的是原生Socket，作为参照
- /apps/webget : 一个简单的http客户端，向服务器发送GET特定URL的HTTP协议

# 使用

编译代码
 `mkdir build`
 `cd build`
 `make -j8` 
运行例程
    配置tun设备及开启路由
     `sudo ../tun.sh start`
 -  1 chargen 带宽测试
    在B主机运行 `sudo nc -l <port> >/dev/null` 
    在A主机运行 
    `./apps/chargen.cc <ip of B> <port>`
    或
    `./apps/chargen_tun.cc <ip of B> <port>`
 -  2 webget
    在B主机编译simple_http_server
    `cd /simple_http_server`
    `go build simple_http_server`
    运行服务器
    `./sudo simple_http_server `
    在A主机运行
    `./apps/webget <ip of B> /hello`
    `./apps/webget <ip of B> /bye`
 -  3 netcat
    在B主机监听某一端口
        `nc -l 8080`
        或
        `./netcat -l 8080`
    从A主机向B主机建立连接
        `nc <ip of B> 8080`
        或
        `./netcat <ip of B> 8080`
    两主机建立了TCP连接，终端内的输入将会被传输给对方

# 结构说明

- /apps : 应用例程
- /etc : CMAKE的宏、C++编译选项、以及tun.sh设备需要用到的地址设置
- /tests : 一些测试
- /tun.sh : 用于开启/关闭tun设备以及对应的端口转发功能
- /lib 
  - / : TCP协议逻辑的主要实现
  - /tcp_helpers : TCP和IP的数据格式和对应的解编码器，TUNSocket的实现
  - /util : address \ buffer \ file descriptors \ parser \ socket \ tun 


# 性能
- 使用chargen \ chargen_tun \ 和nc对比，同时观察CPU占用率，可知用户态TUNSocket的速率实现了原生Socket1/4的性能，这是由于内核态到用户态的复制开销和虚拟网卡引入的额外的路由成本
- 用户态的TCP算法实现仍有优化空间

# API

# 原理


!todo
- 代码注释风格修改
- 代码压缩，去掉没有用到的代码
- 性能调优，看看知乎的回复的人的性能（基于环形缓冲的性能如何）(以及BUFFER的性能如何)
- 测试例程
- 补充测试
- readme写完
- 画几个简单的原理图
- 修复只能开一个tunsocket的问题
- 实现一个简单的VPN协议
- 修复localhost不好用的问题