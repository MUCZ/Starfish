#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer



using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // return WrappingInt32( ((n & 0xFFFFFFFF)  + isn.raw_value()) & 0xFFFFFFFF);
    // return isn + (n & 0xFFFFFFFF);
    return isn + n;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t rest = ((n.raw_value() + 1) +
                     (0xFFFFFFFF - isn.raw_value()));  // why+1?  because ?1 - 3 = ?1 + 10 - 3 = ?8 and 10 = 9 + 1
    uint32_t checkpoint_32 = checkpoint;               // checkpoint & 0xFFFFFFFF;
    uint32_t dif = checkpoint_32 > rest ? checkpoint_32 - rest : rest - checkpoint_32;
    uint64_t indexes;
    if (dif & 0x80000000) {  // bigger than half of uint32
        if (checkpoint_32 < rest && (checkpoint >> 32) > 1)
            indexes = (((checkpoint >> 32) - 1) << 32) + rest;
        else if (checkpoint_32 > rest)
            indexes = (((checkpoint >> 32) + 1) << 32) + rest;
        else
            indexes = (((checkpoint >> 32)) << 32) + rest;
    } else {
        indexes = ((checkpoint >> 32) << 32) + rest;
    }
    return indexes;
}
// n  = 1 , isn = 3, rest = 8, checkpoint = 14, checkpoint_1 = 4, dif = 8-4=4<5, indexes = 10 + 8 = 18
// n  = 9 , isn = 3, rest = 6, checkpoint = 14, checkpoint_1 = 4, dif = 6-4=2<5, indexex = 10 + 6 = 16
// n  = 1 , isn = 3, rest = 8, checkpoint = 12, checkpoint_1 = 2, dif = 8-2=6>5, indexes = 0 + 8 = 8

// int main(){
//     cout<< wrap(4,WrappingInt32(pow(2,32)-2)).raw_value()<<endl;
// }
