#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) 
  : capacity_( capacity ), ring_buffers_{ RingBuffer(capacity + 1), RingBuffer(capacity + 1) }
{}

void Writer::push( string data )
{
  auto & ring_buffer = ring_buffers_[current_ring_buffer_index_];
  uint32_t data_length_can_be_pushed = std::min(static_cast<uint32_t>(data.size()), ring_buffer.availableSize());
  bytes_pushed_ += data_length_can_be_pushed;
  ring_buffer.push(data, data_length_can_be_pushed);
}

void Writer::close()
{
  // Your code here.
  flags_ &= EOF_FLAG;
}

void Writer::set_error()
{
  // Your code here.
  flags_ &= ERROR_FLAG;
}

bool Writer::is_closed() const
{
  // Your code here.
  return (flags_ & EOF_FLAG);
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return ring_buffers_[current_ring_buffer_index_].availableSize();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  auto & old_ring_buffer = ring_buffers_[current_ring_buffer_index_];
  auto & new_ring_buffer = ring_buffers_[1 - current_ring_buffer_index_];
  old_ring_buffer.copyAndRestructure(new_ring_buffer);
  current_ring_buffer_index_ = 1 - current_ring_buffer_index_;
  return new_ring_buffer.toStringView();
}

bool Reader::is_finished() const
{
  // Your code here.
  return (flags_ & EOF_FLAG) && (ring_buffers_[current_ring_buffer_index_].isEmpty());
}

bool Reader::has_error() const
{
  // Your code here.
  return (flags_ & ERROR_FLAG);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  auto & ring_buffer = ring_buffers_[current_ring_buffer_index_];
  ring_buffer.read_index_ = (ring_buffer.read_index_ + len) % ring_buffer.bufferSize(); 
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return ring_buffers_[current_ring_buffer_index_].occupiedSize();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_poped_;
}
