#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if(is_last_substring) {
    reach_end_ = true;
  }

  if(sliding_window_.empty()) {
    buffer_size_ = output.available_capacity();
    sliding_window_.resize(buffer_size_);
    bitmap_.resize(buffer_size_);
  }

  // RAII class to execute the close function
  TaskExecutionScopeGuard guard([this, &output](){
    if(this -> reach_end_ && this -> bytes_pending_ == 0) {
      output.close();
    }
  });

  uint64_t data_tail_index = std::min(first_index + data.size(), first_unassembled_global_index_ + output.available_capacity());

  // data cannot exceed the capacity
  if((first_index >= data_tail_index) || (first_unassembled_global_index_ >= data_tail_index)) {
    // discard
    return;
  }
  if(first_index <= first_unassembled_global_index_) {
    // push to the stream

    // Older version:
    // uint64_t i = first_unassembled_global_index_;

    // for(; i != data_tail_index; i++) {
    //   uint64_t window_index = i % buffer_size_;
    //   if(bitmap_.get(window_index)) {
    //     bytes_pending_--;
    //     bitmap_.unset(window_index);
    //   }
    // }

    // Newer version(using batch processing):
    bytes_pending_ -= bitmap_.getPopCountAndUnsetBatch(first_unassembled_global_index_ % buffer_size_, (data_tail_index + buffer_size_ - 1) % buffer_size_);

    uint64_t data_start_str_index = first_unassembled_global_index_ - first_index;
    first_unassembled_global_index_ = data_tail_index;

    // Below can be optimized to batch processing, if the data feature supports that optimization
    uint64_t i = data_tail_index;
    uint64_t window_index = i % buffer_size_;
    while(bitmap_.get(window_index)) {
      bytes_pending_--;
      bitmap_.unset(window_index);
      data.push_back(sliding_window_[window_index]);
      i++;
      first_unassembled_global_index_++;
      window_index = i % buffer_size_;
    }
    output.push(data.substr(data_start_str_index));
  } else {
    // buffered in the reassembler
    for(uint64_t i = 0; i < data_tail_index - first_index; i++) {
      uint64_t window_index = (i + first_index) % buffer_size_;
      if(!bitmap_.get(window_index)) {
        bytes_pending_++;
        bitmap_.set(window_index);
        sliding_window_[window_index] = data[i];
      }
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
