#include "socket.hh"
#include "tcp_tun_socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {

    TUNSocket sock{};
    sock.connect(Address(host, "http"));
    sock.write("GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n");
    sock.shutdown(SHUT_WR);
    while (!sock.eof()) {
        cout << sock.read();
    }
    sock.wait_until_closed();
    return;
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  
        }

        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << "www.gitee.com /mucz\n";
            return EXIT_FAILURE;
        }

        const string host = argv[1];
        const string path = argv[2];

        get_URL(host, path);

    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
