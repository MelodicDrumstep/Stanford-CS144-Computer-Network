#include <limits>

#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // Referenced from https://github.com/comzyh/TCP-Lab/blob/comzyh/libsponge/wrapping_integers.cc
  // This method is awesome
  Wrap32 wrap_checkpoint = wrap(checkpoint, zero_point);
  uint32_t diff = raw_value_ - wrap_checkpoint.getRawValue();

  if (diff & 0x80000000 && diff + checkpoint <= std::numeric_limits<uint32_t>::max()) {
    // diff & 0x80000000 means the highest bit for diff is 1. 
    // Therefore it's negative if converted to int32_t
    // In this situation, we cannot direcly return checkpoint + static_cast<int32_t>(diff)
    // otherwise the result would overflow
    // We should use diff as uint32_t here
    return checkpoint + diff;
  }
  return checkpoint + static_cast<int32_t>(diff);
}