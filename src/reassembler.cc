#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if(is_last_substring) {
    reach_end_ = true;
  }

  TaskExecutionScopeGuard guard([this, &output](){
    if(this -> reach_end_ && this -> stored_strings_.empty()) {
      output.close();
    }
  });

  uint64_t data_tail_index = std::min(first_index + data.size(), first_unassembled_index_ + output.available_capacity());

  // data cannot exceed the capacity
  if((first_index >= data_tail_index) || (first_unassembled_index_ >= data_tail_index)) {
    // discard
    return;
  }
  if(first_index <= first_unassembled_index_) {
    // push to the stream
    std::string data_to_be_pushed = data.substr(first_unassembled_index_ - first_index);
    first_unassembled_index_ = data_tail_index;

    auto it = stored_strings_.begin();
    // skip the blocks whose tail is not larger than data_tail_index
    while((it != stored_strings_.end()) && (it -> first_index + it -> data.size() <= data_tail_index)) {
      bytes_pending_ -= it -> data.size();
      ++it;
      stored_strings_.pop_front();
    }
    if((it != stored_strings_.end()) && (it -> first_index <= data_tail_index)) {
      // The first one must be handled specifically, because
      // data_tail_index may lie inside the block
      uint64_t last_tail_index = it -> first_index + it -> data.size();
      data_to_be_pushed += it -> data.substr(std::max(0ul, data_tail_index - it -> first_index));
      bytes_pending_ -= it -> data.size();
      ++it;
      stored_strings_.pop_front();
      // In my implementation, for performance considerations,
      // I don't always combine adjacent block together into one.
      // Therefore I have to deal with the adjacent block cases when pushing
      // elements into the stream
      while((it != stored_strings_.end()) && (it -> first_index == last_tail_index)) {
        last_tail_index = it -> first_index + it -> data.size();
        data_to_be_pushed += it -> data;
        bytes_pending_ -= it -> data.size();
        ++it;
        stored_strings_.pop_front();
      }
      first_unassembled_index_ = last_tail_index;
    }
    output.push(data_to_be_pushed);
  } else {
    // buffered in the reassembler
    auto it = stored_strings_.begin();
    // skip the blocks which lies ahead of this block
    while((it != stored_strings_.end()) && (it -> first_index + it -> data.size() <= first_index)) {
      ++it;
    }

    if(it != stored_strings_.end()) {
      uint64_t data_start_index = first_index;
      if(it -> first_index <= first_index) {
        uint64_t it_tail_index = it -> first_index + it -> data.size();
        if(it_tail_index >= data_tail_index) {
          // the input data is enclosed by another data block, no need to do anything
          return;
        }
        data_start_index = it_tail_index;
        // equal to modifying the first_index
        ++it;
      }

      std::string str_block = data.substr(data_start_index - first_index);
      while((it != stored_strings_.end()) && (it -> first_index + it -> data.size() <= data_tail_index)) {
        auto old_it = it;
        ++it;
        bytes_pending_ -= old_it -> data.size();
        stored_strings_.erase(old_it);
      }
      if((it != stored_strings_.end()) && (it -> first_index <= data_tail_index)) {
        auto old_it = it;
        str_block += it -> data.substr(data_tail_index - it -> first_index);
        bytes_pending_ -= it -> data.size();
        ++it;
        stored_strings_.erase(old_it);
      }
      bytes_pending_ += str_block.size();
      stored_strings_.emplace(it, data_start_index, str_block);
    } else {
      // reach the end, directly pushing the block to the tail of the list

      bytes_pending_ += data_tail_index - first_index;
      stored_strings_.emplace_back(first_index, data.substr(0, data_tail_index - first_index));
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
