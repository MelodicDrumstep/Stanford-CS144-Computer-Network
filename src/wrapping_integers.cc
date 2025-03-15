
// TODO: Delete this after testing
#include <iostream>
#include <cmath>

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
  constexpr uint64_t TwoPower32 = 1ULL << 32;
  uint32_t zero_point_raw_value = zero_point.getRawValue();

  // DEBUGING
  std::cout << "[Wrap32::unwrap] raw_value_ is " << raw_value_ << ", zero_point_raw_value is " << zero_point_raw_value << "\n";
  std::cout << "raw_value_ - zero_point_raw_value is " << raw_value_ - zero_point_raw_value << "\n";
  std::cout << "TwoPower32 is " << TwoPower32 << std::endl;
  std::cout << "checkpoint + zero_point_raw_value - raw_value_ is " << checkpoint + zero_point_raw_value - raw_value_ << "\n";
  std::cout << "(checkpoint + zero_point_raw_value - raw_value_ + TwoPower32) is " << (checkpoint + zero_point_raw_value - raw_value_ + TwoPower32) << "\n";
  std::cout << "(checkpoint + zero_point_raw_value - raw_value_ + TwoPower32) / TwoPower32 is " << (checkpoint + zero_point_raw_value - raw_value_ + TwoPower32) / TwoPower32 << "\n";
  // DEBUGING

  uint64_t helper = (zero_point_raw_value > raw_value_) ? zero_point_raw_value - raw_value_ : raw_value_ - zero_point_raw_value;
  uint64_t n = (checkpoint - helper + TwoPower32) / TwoPower32;
  uint64_t upper_bound_ans = raw_value_ - zero_point_raw_value + TwoPower32 * n;
  uint64_t lower_bound_ans = raw_value_ - zero_point_raw_value + TwoPower32 * (n - 1);

  std::cout << "checkpoint is " << checkpoint << "\n";
  std::cout << "n is " << n << "\n";
  std::cout << "upper_bound_ans is " << upper_bound_ans << "\n";
  std::cout << "lower_bound_ans is " << lower_bound_ans << "\n";

  return ((n == 0) || ((upper_bound_ans - checkpoint) <= (checkpoint - lower_bound_ans))) ? upper_bound_ans : lower_bound_ans;
}