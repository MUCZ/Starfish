#include "tcp_sponge_socket.hh"
#include "util.hh"

#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include "socket.hh"

using namespace std;
using namespace std::chrono;

string message;
const int batch = 100000;
int main(int argc,char* argv[]){
    if(argc > 2){

        for(int i = 33; i<2033; ++i){
            message.push_back(static_cast<char>(i));
        }

        string host = string(argv[1]);
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        TCPSocket sock;

        sock.set_nodelay();
        const auto first_time = high_resolution_clock::now();


        sock.connect(Address(host,port));
        for(int i = 0; i < batch ; ++i){
            sock.write(message); 
        }
        sock.close();

        const auto final_time = high_resolution_clock::now();

        const auto duration = duration_cast<nanoseconds>(final_time - first_time).count();
        
        int len = message.size();
        const auto gigabits_per_second = len * 8.0 * batch/ double(duration);
        const auto megabits_per_second = gigabits_per_second * 1000;
        cout << fixed << setprecision(2);
        cout << "limited throughput " << megabits_per_second << " Mbit/s" << " = " << megabits_per_second/8 << " MB/s"<<endl;
        return 1;

    } else {
        cout << "Usage: "<< argv[0]<<" <ip> <port> "<<endl;
    }
}