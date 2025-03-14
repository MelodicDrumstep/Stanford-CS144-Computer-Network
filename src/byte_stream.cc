#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) 
  : capacity_( capacity ), buffer_(capacity)
{}

void Writer::push( string data )
{
  uint64_t len_pushed = std::min(data.size(), available_capacity());
  bytes_pushed_ += len_pushed;
  buffer_.push(data, len_pushed);
}

void Writer::close()
{
  // Your code here.
  flags_ |= EOF_FLAG;
}

void Writer::set_error()
{
  // Your code here.
  flags_ |= ERROR_FLAG;
}

bool Writer::is_closed() const
{
  // Your code here.
  return (flags_ & EOF_FLAG);
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return buffer_.availableCapacity();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  return buffer_.toStringView();
}

bool Reader::is_finished() const
{
  // Your code here.
  return (flags_ & EOF_FLAG) && (buffer_.isEmpty());
}

bool Reader::has_error() const
{
  // Your code here.
  return (flags_ & ERROR_FLAG);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t len_popped = std::min(len, bytes_buffered());
  bytes_popped_ += len_popped;
  buffer_.pop(len_popped);
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.bytesBuffered();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}