#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    size_t len = data.size();
    len = min(len, remaining_capacity());
    _bytes_written += len;
    if(len == data.size()){
        _buffer.emplace_back(move(Buffer(move(string(data))))); // copy here
    } else {
        _buffer.emplace_back(move(data.substr(0,len)));
    }
    return len;
}

size_t ByteStream::write(string &&data) {
    size_t len = data.size();
    len = min(len, remaining_capacity());
    _bytes_written += len;
    if(len == data.size()){
        _buffer.emplace_back(move(BufferPlus(move(data)))); // forward rvalue
    } else {
        _buffer.emplace_back(move(data.substr(0,len)));
    }
    return len;
}

size_t ByteStream::write(BufferPlus& data) { // ! avoid copy
    size_t len = data.size();
    len = min(len, remaining_capacity());
    _bytes_written += len;
    if(len != data.size()){
        data.remove_suffix(data.size()-len);
    }
    if(data.size())
        _buffer.emplace_back(move(data)); 
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t len_ = min(len, buffer_size());
    string ret;
    ret.reserve(len_);
    for (const auto &buffer : _buffer) {
        if (len_ >= buffer.size()) {
            ret.append(buffer); // ! avoid copy 
            // ret.append(move(string().assign(move(buffer.str())))); // !slow 
            len_ -= buffer.size();
            if (len_ == 0)
                break;
        } else {
            // ret.append(string().assign(buffer.str().begin(), buffer.str().begin() + len_)); //  ! fast 
            // ret.append(move(buffer.copy().substr(0,len_))); // ! really slow
            BufferPlus tmp(buffer); // !fastest
            tmp.remove_suffix(buffer.size()-len_);
            ret.append(tmp);
            break;
        }
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t len_ = min(len, buffer_size());
    _bytes_read += len_;
    while (len_ > 0) {
        if (len_ > _buffer.front().size()) {
            len_ -= _buffer.front().size();
            _buffer.pop_front();
        } else {
            _buffer.front().remove_prefix(len_);
            break;
        }
    }
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
string ByteStream::read(const size_t len) {
    string ans = peek_output(len);
    pop_output(len);
    return ans;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _bytes_written - _bytes_read; }

bool ByteStream::buffer_empty() const { return _bytes_written - _bytes_read == 0; }

bool ByteStream::eof() const { return _input_ended && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
