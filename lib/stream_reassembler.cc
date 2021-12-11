#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _bytes_waiting(0)  // = first unassembled
    , _unassembled_byte(0)
    , _eof_set(false)
    , _eof_(0)
    , _blocks() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index + data.size() < _bytes_waiting) {  // data out of date
        return;
    }

    if (eof && !_eof_set) {  // store the eof
        _eof_set = true;
        _eof_ = index + data.size();
    }

    // if the block has prev data, reject that part
    // Bytes that would exceed the capacity are silently discarded.
    StreamBlock blk(index, move(string(data)));
    if (index < _bytes_waiting) {  // has prev data
        blk.buffer().remove_prefix(_bytes_waiting - index);
    }
    if (index + data.size() > _capacity + _bytes_waiting) {  //  exceed the capacity
        blk.buffer().remove_suffix(index + data.size() - _capacity - _bytes_waiting);
    }

    Add_block(blk);
    Write_to_stream();
    EOFcheck();
}

void StreamReassembler::push_substring(const Buffer &data, const size_t index, const bool eof) {
    if (index + data.size() < _bytes_waiting) {  // data out of date
        return;
    }

    if (eof && !_eof_set) {  // store the eof
        _eof_set = true;
        _eof_ = index + data.size();
    }

    // if the block has prev data, reject that part
    // Bytes that would exceed the capacity are silently discarded.
    StreamBlock blk(index, data);
    if (index < _bytes_waiting) {  // has prev data
        blk.buffer().remove_prefix(_bytes_waiting - index);
    }
    if (index + data.size() > _capacity + _bytes_waiting) {  //  exceed the capacity
        blk.buffer().remove_suffix(index + data.size() - _capacity - _bytes_waiting);
    }

    Add_block(blk);
    Write_to_stream();
    EOFcheck();
}


// Check if eof is written to the stream
inline void StreamReassembler::EOFcheck() {
    if (!_eof_set)
        return;
    if (static_cast<size_t>(_eof_) == _bytes_waiting) {
        _output.end_input();
    }
}

// Write the first block to the stream,
//  this block should begin at  '_bytes_waiting'
inline void StreamReassembler::Write_to_stream() {
    while (!_blocks.empty()) {
        auto to_write = *_blocks.begin();
        if (to_write.begin() != _bytes_waiting)
            return;
        // size_t bytes_written = _output.write(move(to_write.buffer().copy())); // ! copy here
        size_t bytes_written = _output.write(to_write.buffer());

        if (bytes_written == 0)
            return;
        _bytes_waiting += bytes_written;
        _unassembled_byte -= bytes_written;
        _blocks.erase(_blocks.begin());        
        if (bytes_written != to_write.len()) {  // partially written
            to_write.buffer().remove_prefix(bytes_written);
            _blocks.insert(to_write);
        } 
    }
}

// add "to_add" blocks to set blocks
// merge all the blocks mergeable
inline void StreamReassembler::Add_block(StreamBlock &new_block) {
    if (new_block.len() == 0)
        return;
    vector<StreamBlock> blks_to_add;
    blks_to_add.emplace_back(new_block);
    do {
        if (_blocks.empty())
            break;
        // compare with nexts
        auto nblk = blks_to_add.begin();
        auto iter = _blocks.lower_bound(*nblk);
        auto prev = iter;
        while (iter != _blocks.end() && overlap(*iter, *nblk)) {
            if ((*iter).end() >= (*nblk).end()) {
                (*nblk).buffer().remove_suffix((*nblk).end() - (*iter).begin());
                break;
            } else {
                StreamBlock last(*nblk);
                (*nblk).buffer().remove_suffix((*nblk).end() - (*iter).begin());
                last.buffer().remove_prefix((*iter).end() - (*nblk).begin());
                blks_to_add.push_back(last);
                nblk = blks_to_add.end();
                nblk--;
                iter++;
            }
        }
        // compare with prevs
        if (prev == _blocks.begin())  //  check one previous block is enough
            break;
        prev--;
        nblk = blks_to_add.begin();
        if (overlap(*nblk, *prev)) {
            (*nblk).buffer().remove_prefix((*prev).end() - (*nblk).begin());
        }
    } while (false);

    for (auto &b : blks_to_add) {
        if (b.len() != 0) {
            _blocks.emplace(b);
            _unassembled_byte += b.len();
        }
    }
}

bool StreamReassembler::overlap(const StreamBlock &blk, const StreamBlock &new_blk) const {
    if (blk.begin() < new_blk.begin()) {
        return overlap(new_blk, blk);
    }
    return !(blk.begin() >= new_blk.end());
}

uint64_t StreamReassembler::first_unassembled() const { return _bytes_waiting; }

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_byte; }

bool StreamReassembler::empty() const { return _unassembled_byte == 0; }

