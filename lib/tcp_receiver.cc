#include "tcp_receiver.hh"

#include "util.hh"

// Dummy implementation of a TCP receiver


using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!_ackno.has_value()) {
        if (seg.header().syn) {  // handshake
            auto rd = get_random_generator();
            _isn = WrappingInt32(rd());
            sender_isn = seg.header().seqno;
            _reassembler.push_substring((seg.payload().copy()), 0, seg.header().fin);
            _ackno = WrappingInt32(sender_isn) + (seg.length_in_sequence_space() - seg.payload().size()) +
                     _reassembler.first_unassembled();  // SYN or FIN make _ackno+1
        }
        return;
    }
    if (_ackno.has_value() && !stream_out().input_ended()) {
        WrappingInt32 seqno = seg.header().seqno;
        uint64_t index = unwrap(seqno, sender_isn + 1, _checkpoint);        // "+ 1" for the "SYN"
        if (index > _checkpoint && ((index - _checkpoint) & 0x80000000)) {  // data too far, considered out of data
            // cerr<< "-DEBUG: receive abnormal seqno, discard"<<endl;
            return;
        }
        if (index < _checkpoint && ((_checkpoint - index) & 0x80000000)) {  // data too far, considered out of data
            // cerr<< "-DEBUG: receive abnormal data, discard"<<endl;
            return;
        }
        // _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin); // ! copy here
        _reassembler.push_substring(Buffer(move(seg.payload().copy())), index, seg.header().fin); // ! copy here
        // _reassembler.push_substring(seg.payload(),index, seg.header().fin);

        _ackno = _ackno.value() + _reassembler.first_unassembled() - _checkpoint;
        if (stream_out().input_ended()) {  // FIN should make _ackno+1
            _ackno = _ackno.value() + 1;
            // cerr<< "DEBUG: FIN_RECEIVED"<<endl;
        }
        _checkpoint = _reassembler.first_unassembled();
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { return _ackno; }

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
