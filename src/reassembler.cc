#include "reassembler.hh"

// TODO: Delete this after testing
#include <iostream>
#include <bitset>
#include <iomanip>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // // TODO: Delete this after testing
  // std::cout << "[insert] " << "first_index : " <<  first_index << ", " << "data : ";
  // #ifdef OutputHex 
  //   for(auto c : data) {
  //     std::cout << std::hex << std::setw(2) << std::setfill('0') 
  //     << static_cast<int32_t>(c) << " ";
  //   }
  // #else
  //   std::cout << data;
  // #endif
  // std::cout << ", " << "is_last_substring :  " << is_last_substring << std::endl;
  // std::cout << "stored_strings_: ";
  // for(auto it = stored_strings_.begin(); it != stored_strings_.end(); ++it) {
  //   std::cout << "[" << it -> first_index << ", ";
  //   #ifdef OutputHex 
  //     for(auto c : it -> data) {
  //       std::cout << std::hex << std::setw(2) << std::setfill('0') 
  //       << static_cast<int32_t>(c) << " ";
  //     }
  //   #else
  //     std::cout << it -> data;
  //   #endif
  //   std::cout << "] ";
  // }
  // std::cout << std::endl;
  // // DEBUGING

  if(is_last_substring) {
    reach_end_ = true;
  }

  TaskExecutionScopeGuard guard([this, &output](){
    if(this -> reach_end_ && this -> stored_strings_.empty()) {
      output.close();
    }
  });

  uint64_t data_tail_index = std::min(first_index + data.size(), first_unassembled_index_ + output.available_capacity());
  
  // // TODO: Delete this after testing
  // std::cout << "data_tail_index: " << data_tail_index << ", " << "first_unassembled_index_: " 
  //   << first_unassembled_index_ << ", output.available_capacity(): " << output.available_capacity() << std::endl;
  // // DEBUGING

  // data cannot exceed the capacity
  if((first_index >= data_tail_index) || (first_unassembled_index_ >= data_tail_index)) {
    // discard
    // // TODO: Delete this after testing
    // std::cout << "[insert] " << "discard" << std::endl;
    // // DEBUGING
    return;
  }
  if(first_index <= first_unassembled_index_) {
    // push to the stream
    // // TODO: Delete this after testing
    // std::cout << "[insert] " << "first_index <= first_unassembled_index_" << std::endl;
    // // DEBUGING
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
    // // TODO: Delete this after testing
    // std::cout << "[insert] " << "output.push \"";
    // #ifdef OutputHex 
    //   for(auto c : data_to_be_pushed) {
    //     std::cout << std::hex << std::setw(2) << std::setfill('0')
    //     << static_cast<int32_t>(c) << " ";
    //   }
    // #else
    //   std::cout << data_to_be_pushed;
    // #endif
    // std::cout << "\"" << std::endl;
    // // DEBUGING
  } else {
    // buffered in the reassembler
    // // TODO: Delete this after tetsing
    //   std::cout << "buffered in the reassembler\n";

    auto it = stored_strings_.begin();
    // skip the blocks which lies ahead of this block
    while((it != stored_strings_.end()) && (it -> first_index + it -> data.size() <= first_index)) {
      ++it;
    }
    // DEBUGING
    // std::cout << "after skiping the blocks which lies ahead of this block, bytes_pending_ is " << bytes_pending_ << std::endl;
    // DEBUGING

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

      // TODO: Delete this after tetsing
      // std::cout << ""

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

        // // DEBUGING
        // std::cout << "after -= (it -> data.size()), bytes_pending_ is " << bytes_pending_ << "\n";
        // // DEBUGING

        ++it;
        stored_strings_.erase(old_it);
      }
      // // DEBUGING
      // std::cout << "emplace, data_start_index is " << data_start_index << ", str_block is " << str_block << "\n";
      // std::cout << "bytes_pending_ is " << bytes_pending_ << std::endl;
      // // DEBUGING
      
      bytes_pending_ += str_block.size();
      stored_strings_.emplace(it, data_start_index, str_block);
    } else {
      // reach the end, directly pushing the block to the tail of the list

      bytes_pending_ += data_tail_index - first_index;
      stored_strings_.emplace_back(first_index, data.substr(0, data_tail_index - first_index));
    }

  // // TODO: Delete this after testing
  // std::cout << "[After insert] stored_strings_ : \n";
  // for(auto it_p = stored_strings_.begin(); it_p != stored_strings_.end(); ++it_p) {
  //   std::cout << "[" << it_p -> first_index << ", ";
  //   #ifdef OutputHex 
  //     for(auto c : it_p -> data) {
  //       std::cout << std::hex << std::setw(2) << std::setfill('0') 
  //       << static_cast<int32_t>(c) << " ";
  //     }
  //   #else
  //     std::cout << it_p -> data;
  //   #endif
  //   std::cout << "] ";
  // }
  // std::cout << std::endl;
  // // DEBUGING
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
