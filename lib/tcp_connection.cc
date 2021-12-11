#include "tcp_connection.hh"

#include <iostream>


using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _curr_time - _last_segment_time; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    // cerr << "-DEBUG: receive segment with" << seg.header().summary()  <<endl;

    _last_segment_time = _curr_time;
    if (seg.header().rst) {  // Unclean shutdown of TCPConnection
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
    } else {  // normal routine
        _receiver.segment_received(seg);
        if (seg.header().ack) {
            _sender.ack_received(seg.header().ackno, seg.header().win);
        }
        if (_receiver.ackno().has_value()) {  // syn received
            send_segment();
            _sender.fill_window();
            if (seg.length_in_sequence_space() &&
                _sender.segments_out().empty()) {  // at least one segment is sent in reply
                // cerr << "-DEBUG: send empty ACK " << endl;
                _sender.send_empty_ack();
            }
            send_segment();
        }
        if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
            // cerr << "-DEBUG: _linger_after_streams_finish set to false " << endl;
            _linger_after_streams_finish = false;
        }
    }
}

void TCPConnection::send_segment() {
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        seg.header().win =
            static_cast<uint16_t>(min(_receiver.window_size(), static_cast<size_t>(numeric_limits<uint16_t>::max())));
        //  cerr << "-DEBUG: send segment with  " << seg.header().summary()  <<endl;
        _segments_out.push(move(seg));
    }
}

bool TCPConnection::active() const {
    if (_sender.stream_in().error() || _receiver.stream_out().error())  // unclean shutdown
        return false;
    if (!_linger_after_streams_finish) {  // clean shut down
        if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() &&
            _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
            _sender.bytes_in_flight() == 0) {  // # 1 ~ # 3 satisfied ->connection done
            // cerr << "-DEBUG: 1~3 satified" <<endl;
            return false;
        } else {
            return true;
        }
    } else {
        if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() &&
            _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
            _sender.bytes_in_flight() == 0) {
            if (time_since_last_segment_received() < 10 * _cfg.rt_timeout)
                return true;
            else {
                return false;
                // cerr << "-DEBUG: no message since "<< 10*_cfg.rt_timeout << "ms, session end" <<endl;
            }
        } else {
            return true;
        }
    }
}

size_t TCPConnection::write(const string &data) {
    size_t bytes_written = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return bytes_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _curr_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    send_segment();
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        _sender.send_empty_rst();  // abort the connnection
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        send_segment();
        return;
    } else if (_receiver.ackno().has_value()) {  // syn received
        _sender.fill_window();
    }
    send_segment();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}

void TCPConnection::connect() {
    _sender.fill_window();
    send_segment();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_rst();
            send_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
