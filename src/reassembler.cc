#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  uint64_t data_tail_index = std::min(first_index + data.size(), unassembled_index_ + output.available_capacity());
  // data cannot exceed the capacity
  if(first_index >= data_tail_index) {
    // discard
    return;
  }
  if(first_index <= first_unassembled_index_) {
    // push to the stream
    std::string data_to_be_pushed = data.substr(first_unassembled_index_, data_tail_index - first_unassembled_index_);
    first_unassemled_index_ = data_tail_index;
    auto it = stored_strings_.begin();
    // skip the blocks whose tail is not larger than data_tail_index
    while((it != stored_strings_.end()) && (it_tail_index <= data_tail_index)) {
      bytes_pending_ -= it -> data.size();
      stored_index_.pop_front();
      ++it;
    }
    if((it != stored_strings_.end()) && (it -> first_index <= data_tail_index)) {
      // The first one must be handled specifically, because
      // data_tail_index may lie inside the block
      uint64_t last_tail_index = it -> first_index + it -> data.size();
      data_to_be_pushed += it -> data.substr(std::min(0, data_tail_index - it -> first_index));
      bytes_pending_ -= it -> data.size();
      stored_index_.pop_front();
      it++;
      // In my implementation, for performance considerations,
      // I don't always combine adjacent block together into one.
      // Therefore I have to deal with the adjacent block cases when pushing
      // elements into the stream
      while(it -> first_index == last_tail_index) {
        last_tail_index = it -> first_index + it -> data.size();
        data_to_be_pushed += it -> data;
        bytes_pending_ -= it -> data.size();
        stored_index_.pop_front();
        it++;
      }
    }
    output.push(data_to_be_pushed);
  } else {
    // buffered in the reassembler
    auto it = stored_strings_.begin();
    // skip the blocks which lies ahead of this block
    while(it -> first_index + it -> data.size() < first_index) {
      ++it;
    }
    // 
    if(it != stored_strings_.end()) {
      if(it -> first_index < first_index) {
        it -> data.resize(first_index - it -> first_index);
        ++it;
      } else if(it -> first_index == first_index) {
        if(it -> frist_index + it -> data.size() >= data_tail_index) {
          return;
        }
        else {
          // TO BE CONTINUED
        }
      }
    } else {
      // directly pushing the block to the tail of the list
      storedd_index_.emplace_back(data.substr(first_unassembled_index_, data_tail_index - first_unassembled_index_));
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
