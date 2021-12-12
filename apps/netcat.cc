// A dummy implementation of netcat using TUNSocket
#include "socket.hh"
#include "tcp_sponge_socket.hh"
#include "util.hh"

#include <csignal>
#include <assert.h>
#include <thread>
#include <cstdlib>
#include <iostream>

using namespace std;
TUNSocket *g_sock{nullptr};

void read_from_peer(){ 
    assert(g_sock!=nullptr);
    while(g_sock){
        auto line = g_sock->read();
        if(line.size()){
            if((line.back()) == '\n'){
                line.pop_back();
            }
            cout<<">> "<<line<<endl;
        }
    }
}
void send_to_peer(const string &host, const uint16_t port) {
    TUNSocket sock{};
    g_sock = &sock;
    sock.connect(Address(host,port));
    string line;
    thread t_(&read_from_peer);
    while(getline(cin,line)){
        sock.write(line+'\n');
    }
}


void listen(const uint16_t port){ 
    TUNSocket sock;
    TCPConfig tcp_config;
    tcp_config.rt_timeout = 100;
    FdAdapterConfig multiplexer_config;
    multiplexer_config.source = {"169.254.144.2",to_string(port)};
    sock.listen_and_accept(tcp_config, multiplexer_config);
    g_sock = &sock;
    thread t_(&read_from_peer); 
    string line;
    while( getline(cin,line)){
        sock.write(line+'\n');
    }
}

void sigintHandler(int signum){ 
    (void)signum;
    cout << "\nInterrupt signal received."<<endl;
    if(g_sock){
        // g_sock->shutdown(SHUT_WR);
        g_sock->wait_until_closed();
    }
    exit(1);
}

void sigpipeHandler(int signum){
    (void)signum;
    cout << "\nConnection closed by peer."<<endl;
    exit(1);
}

void watchdog(){ 
    while(1){
        this_thread::sleep_for(std::chrono::seconds(1));
        if(!g_sock){
            continue;
        } else if(g_sock->in_bound_shutdown() || g_sock->out_bound_shutdown()){
            g_sock->shutdown(SHUT_WR);
            exit(1);
        }
    }
}
int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  
        }
        if (argc != 3  ) {
            cerr << "Usage: " << argv[0] << " <ip> <port> or -l <port>\n"; 
            return EXIT_FAILURE;
        }
        signal(SIGINT,sigintHandler);
        signal(SIGPIPE,sigpipeHandler);
        thread watchdog_thread(watchdog);
        if(string(argv[1])!="-l"){
            const string host = argv[1];
            const uint16_t port = atoi(argv[2]);
            send_to_peer(host, port);
        } else { // -server
            const uint16_t port = atoi(argv[2]);
            listen(port);
        }
        
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
