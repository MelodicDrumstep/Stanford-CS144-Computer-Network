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

  /**
   * A really simple implementation of the bytestream. Just use one flat buffer,
   * and "push" function must make sure the elements are consecutive. If the buffer will overflow 
   * when pushing, firstly considering move elements to the head of the buffer. And resize the 
   * buffer when necessary. 
   * Clearly a lot of optimization space here. But I think most of the overhead comes from the
   * prototype of "peek" function, which requires returning a std::string_view.
   * For real-life local communication, we would use ring buffer (and make it lockfree for multi-threading)
   */
class ByteStream
{
protected:
  uint64_t capacity_;
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  
  int8_t flags_ = 0;
  uint64_t bytes_pushed_ = 0;
  uint64_t bytes_popped_ = 0;
  constexpr static uint32_t EOF_FLAG = 0b01;
  constexpr static uint32_t ERROR_FLAG = 0b10;


  class Buffer
  {
  public:
    Buffer(uint64_t capacity) : capacity_(capacity), array_(capacity) {}

    uint64_t bytesBuffered() const {
      return write_index_ - read_index_;
    }

    uint64_t availableCapacity() const {
      return capacity_ - bytesBuffered();
    }

    void push(const std::string & data, uint64_t len_pushed) {
      if(len_pushed == 0) {
        return;
      }

      uint64_t distance_write_index_to_tail = array_.size() - write_index_;
      uint64_t distance_head_to_read_index = read_index_;
      if(distance_write_index_to_tail >= len_pushed) {
        std::memcpy(&(array_[write_index_]), data.c_str(), len_pushed);
        write_index_ += len_pushed;
        return;
      }
      else if (distance_head_to_read_index + distance_write_index_to_tail < len_pushed) {
        array_.resize(array_.size() * 2);
      } 

      uint64_t bytes_buffered = bytesBuffered();
      if((bytes_buffered > 0) && (read_index_ >= bytes_buffered)) {
        std::memcpy(&(array_[0]), &(array_[read_index_]), bytes_buffered);
      } else {
        for(uint64_t i = 0; i < bytes_buffered; i++) {
          array_[i] = array_[i + read_index_];
        }
      }
      write_index_ = write_index_ - read_index_;
      read_index_ = 0;
      std::memcpy(&(array_[write_index_]), data.c_str(), len_pushed);
      write_index_ = write_index_ + len_pushed;
    }

    void pop(uint64_t len) {
      read_index_ += len;
    }

    bool isEmpty() const {
      return (write_index_ == read_index_);
    }

    std::string_view toStringView() const {
      return std::string_view(array_.data() + read_index_, bytesBuffered());
    }

  private:
    uint64_t capacity_;
    std::vector<char> array_;
    uint64_t write_index_ = 0;
    uint64_t read_index_ = 0;
  };

  Buffer buffer_;

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
