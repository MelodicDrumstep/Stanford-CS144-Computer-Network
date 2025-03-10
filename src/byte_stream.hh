#pragma once

#include <queue>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <array>
#include <string_view>
#include <cstring>

class Reader;
class Writer;

class ByteStream
{
protected:
  uint64_t capacity_;
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  
  int8_t flags_ = 0;
  uint64_t bytes_pushed_ = 0;
  uint64_t bytes_poped_ = 0;
  constexpr static uint32_t EOF_FLAG = 0b01;
  constexpr static uint32_t ERROR_FLAG = 0b10;

  struct RingBuffer
  {
    RingBuffer(const uint32_t buffer_size) : buffer_size_(buffer_size), buffer_(buffer_size) {}
    RingBuffer(const RingBuffer &) = delete;
    RingBuffer(RingBuffer &&) = delete;
    RingBuffer & operator=(const RingBuffer &) = delete;
    RingBuffer & operator=(RingBuffer &&) = delete;

    uint32_t capacity() const {
      return buffer_size_ - 1;
    }
    uint32_t bufferSize() const {
      return buffer_size_;
    }
    uint32_t occupiedSize() const {
      return (write_index_ + buffer_size_ - read_index_) % buffer_size_; 
    }
    uint32_t availableSize() const {
      return (capacity() - occupiedSize());
    }
    uint32_t availableWriteSizeWithoutLoopingBack() const {
      return buffer_size_ - write_index_;
    }
    bool isEmpty() const {
      return (write_index_ == read_index_);
    }
    bool isFull() const {
      return (occupiedSize() == capacity());
    }
    char top() {
      return buffer_[read_index_];
    }
    void pop() {
      read_index_++;
    }
    void push(const std::string & data, uint32_t len) {
      uint32_t data_length_pushed_directly_without_looping_back = std::min(len, availableWriteSizeWithoutLoopingBack());
      std::memcpy(&(buffer_[write_index_]), data.c_str(), data_length_pushed_directly_without_looping_back);
      if(data_length_pushed_directly_without_looping_back < len) {
        std::memcpy(&(buffer_[0]), data.c_str() + data_length_pushed_directly_without_looping_back, len - data_length_pushed_directly_without_looping_back);
      }
      write_index_ = (write_index_ + len) % bufferSize();
    }

    void copyAndRestructure(RingBuffer & other) const {
      if(read_index_ < write_index_) {
        std::memcpy(&(other.buffer_[0]), &(buffer_[read_index_]), write_index_ - read_index_);
      } else if(write_index_ < read_index_) {
        uint32_t read_index_tail_distance = buffer_size_ - read_index_;
        std::memcpy(&(other.buffer_[0]), &(buffer_[read_index_]), read_index_tail_distance);
        std::memcpy(&(other.buffer_[read_index_tail_distance]), &(buffer_[0]), write_index_);
      }

      other.write_index_ = 0;
      other.read_index_ = (write_index_ + buffer_size_ - read_index_) % buffer_size_;
    }

    std::string_view toStringView() const {
      return std::string_view(&(buffer_[0]), occupiedSize());
    }

    uint32_t buffer_size_;
    mutable std::vector<char> buffer_;
    uint32_t write_index_ = 0;
    uint32_t read_index_ = 0;
  };

  std::array<RingBuffer, 2> ring_buffers_;
  uint8_t current_ring_buffer_index_ = 0;

public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // Push data to stream, but only as much as available capacity allows.

  void close();     // Signal that the stream has reached its ending. Nothing more will be written.
  void set_error(); // Signal that the stream suffered an error.

  bool is_closed() const;              // Has the stream been closed?
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now?
  uint64_t bytes_pushed() const;       // Total number of bytes cumulatively pushed to the stream
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // Peek at the next bytes in the buffer
  void pop( uint64_t len );      // Remove `len` bytes from the buffer

  bool is_finished() const; // Is the stream finished (closed and fully popped)?
  bool has_error() const;   // Has the stream had an error?

  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream
};

/*
 * read: A (provided) helper function thats peeks and pops up to `len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t len, std::string& out );
