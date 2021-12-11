#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"
#include "util/buffer.hh"


#include <cstdint>
#include <iostream>
#include <set>
#include <string>
// #include<ext/pool_allocator.h>  


class StreamBlock {
  private:
    BufferPlus _buffer{};
    size_t _begin_index;

  public:
    bool operator<(const StreamBlock rhs) const { return begin() < rhs.begin(); }

    StreamBlock(const int begin, std::string &&str) noexcept : _buffer(std::move(str)), _begin_index(begin){};
    StreamBlock(const StreamBlock &AnotherBlock) noexcept
        : _buffer(AnotherBlock._buffer), _begin_index(AnotherBlock._begin_index){};
    StreamBlock(const int begin, const Buffer &data) noexcept : _buffer(data), _begin_index(begin){};


    // Interface
    inline size_t end() const { return _begin_index + _buffer.statring_offset() + _buffer.size(); }
    inline size_t len() const { return _buffer.size(); }
    inline size_t begin() const { return _begin_index + _buffer.statring_offset(); }
    BufferPlus &buffer() { return _buffer; }
    const BufferPlus &buffer() const { return _buffer; }

};

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;     //!< The reassembled in-order byte stream
    size_t _capacity;       //!< The maximum number of bytes
    size_t _bytes_waiting;  // index of segment waiting for
    size_t _unassembled_byte;
    bool _eof_set;  // whether _eof_ is effecitve
    size_t _eof_;   // where the eof is
    // std::set<StreamBlock,std::less<StreamBlock>,__gnu_cxx::__pool_alloc<StreamBlock>> _blocks;
    std::set<StreamBlock> _blocks;

    //! Merge the two blocks "blk" and "new_block"
    //! the result will stored in new_block
    //! nothing happens if two blocks can't merge
    //! return ture if merge happens, false otherwise
    // inline bool Merge_block(const block &blk, block &new_block);

    //! add "to_add" blocks to set blocks
    //! merge all the blocks mergeable
    inline void Add_block(StreamBlock &new_block);

    bool overlap(const StreamBlock &blk, const StreamBlock &new_blk) const;

    //! Write the first block to the stream, this block should begin at  '_bytes_waiting'
    inline void Write_to_stream();

    //! Check if eof is written to the stream
    //! If true, end the stream
    inline void EOFcheck();

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);
    void push_substring(const Buffer &data, const size_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    uint64_t first_unassembled() const;
    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
