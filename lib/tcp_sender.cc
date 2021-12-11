#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender


template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timeout{retx_timeout}
    , _timer()
    , _window_size(1)
    , _bytes_in_flight(0)
    , _segments_in_flight() {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // to get a ack when window is reopen
    size_t window_size = _window_size == 0 ? 1 : _window_size;
    // first message : SYN
    if (_state == CLOSED) {
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = wrap(_next_seqno, _isn);
        _next_seqno += seg.length_in_sequence_space();
        _bytes_in_flight += seg.length_in_sequence_space();
        _segments_in_flight.push(seg);
        _segments_out.emplace(move(seg));
        // begin timer
        if (!_timer.activated()) {
            _timer.reset(_retransmission_timeout);
            // cerr<< "_retransmission_timeout "<< _retransmission_timeout<<endl;
        }
        _state = SYN_SENT;
        // cerr<<"DEBUG: send SYN "<<endl;
    } else if (_state == SYN_ACKED) {
        // stream ongoing
        size_t send_bytes_count = 0;
        if (_bytes_in_flight >= window_size)
            return;
        size_t max_tobe_send = window_size - _bytes_in_flight;
        while (send_bytes_count < max_tobe_send && !_stream.buffer_empty()) {
            // make up a seg
            TCPSegment seg;
            seg.payload() =
                Buffer(move(_stream.read(min(TCPConfig::MAX_PAYLOAD_SIZE, max_tobe_send - send_bytes_count))));
            send_bytes_count += seg.payload().size();
            if (_stream.eof() && send_bytes_count < max_tobe_send) {
                seg.header().fin = 1;
                _state = FIN_SENT;
                // cerr<<"DEBUG: send FIN "<<endl;
            }
            seg.header().seqno = wrap(_next_seqno, _isn);

            // update
            _next_seqno += seg.length_in_sequence_space();
            _bytes_in_flight += seg.length_in_sequence_space();

            // backup
            _segments_in_flight.push(seg);

            // write to stream
            _segments_out.emplace(move(seg));
            if (!_timer.activated()) {
                _timer.reset(_retransmission_timeout);
            }
        }
        // send FIN when it's not carried in a TCP package
        if (window_size - _bytes_in_flight >= 1 && _stream.eof() && _state == SYN_ACKED) {
            TCPSegment fin_seg;
            fin_seg.header().fin = 1;
            fin_seg.header().seqno = wrap(_next_seqno, _isn);
            _bytes_in_flight += fin_seg.length_in_sequence_space();
            _next_seqno += fin_seg.length_in_sequence_space();
            _segments_in_flight.push(fin_seg);
            _segments_out.emplace(move(fin_seg));
            if (!_timer.activated()) {
                _timer.reset(_retransmission_timeout);
            }
            _state = FIN_SENT;
            // cerr<<"DEBUG: send FIN "<<endl;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if (unwrap(ackno, _isn, _next_seqno) > _next_seqno) {
        // cerr<< "-DEBUG: receive abnormal ackno, discard"<<endl;
        return;
    }
    _window_size = window_size;
    if (_state == SYN_SENT && ackno == wrap(1, _isn)) {
        _state = SYN_ACKED;
        // cerr<< "DEBUG: SYN_SENT -> SYN_ACKED"<<endl;
    }
    if (_segments_in_flight.empty())
        return;
    TCPSegment seg = _segments_in_flight.front();
    bool successful_receipt_of_new_data = false;
    while (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <=
           unwrap(ackno, _isn, _next_seqno)) {
        _bytes_in_flight -= seg.length_in_sequence_space();
        _segments_in_flight.pop();
        successful_receipt_of_new_data = true;
        if (_segments_in_flight.empty())
            break;
        seg = _segments_in_flight.front();
    }
    if (successful_receipt_of_new_data) {  // reset
        _retransmission_timeout = _initial_retransmission_timeout;
        if (!_segments_in_flight.empty()) {
            _timer.reset(_retransmission_timeout);
        } else {
            _timer.stop();
        }
        _consecutive_retransmission_count = 0;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_timer.on_off && _timer.passing(ms_since_last_tick)) {
        // Retransmit the earliest (lowest sequence number) segment
        TCPSegment seg = _segments_in_flight.front();
        if (_window_size != 0) {
            _consecutive_retransmission_count++;
            _retransmission_timeout *= 2;
        }
        if (_consecutive_retransmission_count <= TCPConfig::MAX_RETX_ATTEMPTS) {
            _segments_out.push(seg);
            _timer.reset(_retransmission_timeout);

        } else {
            cerr << "DEBUG: too many retx : send rst " << endl;
            _timer.on_off = false;
            // cerr << "DEBUG: seg info "<< seg.header().summary()<<endl;
            // cerr << "DEBUG: window size "<< _window_size << " bytes in flight "<< _bytes_in_flight <<endl;
        }
        // Reset the retransmission timer and start it
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission_count; }

void TCPSender::send_empty_ack() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    // cerr<< "in empty_ack _time_rest "<< _timer._time_rest<<endl;
    _segments_out.emplace(move(seg));
}

void TCPSender::send_empty_rst() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.header().rst = true;
    _segments_out.emplace(move(seg));
}
