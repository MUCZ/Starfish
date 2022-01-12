# Starfish 
## An Eventloop-based user-level TCP stack.

# 介绍

- 使用C++17，基于事件循环模式的用户态套接字网络编程基础库
- 基于Linux/tun设备直接收发ip数据报，在用户态构建TCP Header, **允许本机发出具有任意IP地址的TCP数据包**
- 实现了TCP协议的主要部分: 自动分包与重组，基于累计确认和超时重传的可靠传输和基于滑动窗口的流量控制 
- 底层使用基于智能指针的的zero-copy Buffer 和 scatter-gather I/O减少冗余拷贝实现高性能

<p align="center">
  <img src="https://github.com/MUCZ/Starfish/blob/main/img/tunsocket.png">
</p>

# 用途
  - **学习用途**，TCP协议\事件循环模式\网络编程\虚拟网络\iptables\隧道代理\现代C++相关
  - 利用TUNSocket的封装，可以**实现OpenVPN等VPN协议,实现IP层级别的隧道代理工具**(与socks,shadowsocks等*socks协议的不同，在于后者是TCP\UDP级别的代理)
    - OpenVPN客户端：将所有流量路由至TUN设备，在应用层处理数据（例如加密）后重新封装流量，经物理网卡发送至代理
    - OpenVPN代理端：由物理网卡接受流量，在应用层处理（例如解密）后重新封装经TUN设备路由至内网,实现虚拟网络加密代理访问
    - 服务端：感知不到代理的存在，认为接受到的客户的IP地址即客户的真实地址 (在*socks协议代理的情况下，服务端收到的ip是代理的ip，因此无法区分同一个代理服务器的多个客户的IP)
      <p align="center">
        <img src="https://github.com/MUCZ/Starfish/blob/main/img/tunsocket_vpn.png">
      </p>
  - 本仓库实现了"my dummy TCP over IP",**可用类似的模式实现'"user-defined protocol" over UDP'**, 与现有网络基础设施兼容的同时使用用户自定义的可靠传输协议，例如，谷歌的QUIC即是这种模式
  - 用于**构建测试程序**，例如构建三次握手的TCP数据包而不用分配缓冲区，**低成本地模拟压力负载或恶意攻击**

# 例程 

- /apps/netcat : 基于TUNSocket的netcat服务，支持与真实nc客户端通信
- /apps/chargen_tun : 基于TUNSocket的chargen服务，主动发起连接后向另一端发送任意字符，发送完成后输出带宽，用于衡量性能
- /apps/chargen : 同上，但使用的是原生Socket，提供性能对照
- /apps/webget : 基于TUNSocket的一个简单的http客户端，向服务器发送GET指定URL的HTTP协议
- /apps/tcp_benchmark : 测试tcp协议实现性能的例程

# 说明

- /apps : 应用例程
- /etc : CMAKE的宏、C++编译选项、以及tun.sh设备需要用到的地址设置
- /tests : 测试夹具和单元测试
- /tun.sh : 用于开启/关闭tun设备以及对应的端口转发和NAT功能
- /lib 
  - / : TCP协议的实现
  - /tcp_helpers : TCP和IP的数据格式和对应的解编码器，TUNSocket的实现
  - /util : address \ buffer \ file descriptors \ parser \ socket \ tun \ syscall \ runtime err 等的封装


# 性能
- 使用仓库内提供的chargen \ chargen_tun 的代码和netcat传输普通文件这三者对比，可知netcat的性能和chargen的性能近似，用户态TUNSocket的实现了原生Socket1/4的性能，这是由于内核态到用户态的复制开销和虚拟网卡引入的额外的路由成本
- 注：由于使用了Linux/tun实现收发ip数据包的方法，这件事本身就会多带来两次数据包的内核态与用户态的数据复制开销，因此该方法的理论最高上限也不会超过原生套接字的一半。
- 详细性能数据和优化方法记录，见我的知乎专栏： https://zhuanlan.zhihu.com/p/414279516

# 使用TUNSocket
- 使用指定的TUNSocket设备名： 
  ```C++
    TUNSocket tunsock0; // 缺省设备名
    TUNSocket tunsock1("tun1"); // 自定义设备名
  ```

- 指定TUNSocket发出的TCP包的源ip地址：
  ```C++
    TUNSocket tunsock1("tun1")
    // tunsock1.connect(dest_ip); // 使用缺省源ip设置
    tunsock1.connect("169.254.144.2",destination_ip) //使用用户指定源ip
  ```

- 设置TUNSocket超时重传时间
   ```C++
      TUNSocket tunsock;
      TCPConfig tcp_config;
      tcp_config.rt_timeout = 100;
      tunsock.connect(tcp_config,destination_ip);
   ```

- 设置TUNSocket缓存区大小 
   ```C++
      TUNSocket tunsock;
      TCPConfig tcp_config;
      tcp_config.recv_capacity = 64000;
      tunsock.connect(tcp_config,destination_ip);
   ```

# 运行例程

编译代码

 ``` bash
 mkdir build
 cd build
 make -j8
 ```

运行例程
    配置tun设备及开启路由

    `sudo ../tun.sh start`

 -  chargen服务，传完完毕后显示运行传输带宽

    在B主机运行

    `sudo nc -l <port> >/dev/null` 

    在A主机运行 

    `./apps/chargen.cc <ip of B> <port>`
    或
    `./apps/chargen_tun.cc <ip of B> <port>`

 -  webget

    在B主机编译simple_http_server

    ``` bash
    cd /simple_http_server
    go build simple_http_server
    ```

    运行服务器
    `./sudo simple_http_server`

    在A主机运行

      `./apps/webget <ip of B> /hello`
      和
      `./apps/webget <ip of B> /bye`

 -  netcat

    在B主机监听某一端口

        `nc -l 8080`
        或
        `./netcat -l 8080`

    从A主机向B主机建立连接

        `nc <ip of B> 8080`
        或
        `./netcat <ip of B> 8080`

    两主机建立了TCP连接，终端内的输入将会被传输给对方
