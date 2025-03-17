#include <random>
#include <deque>
#include <cassert>
#include <iostream>

#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ),
    timer_(initial_RTO_ms)
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return unsent_seqnos_ + unacked_seqnos_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retransmission_num_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  // DEBUGING
  std::cout << "[TCPSender::maybe_send], unsent_seqnos_ is " << unsent_seqnos_
    << ", unacked_seqnos_ is " << unacked_seqnos_ << "\n";
  std::cout << "has_not_sent_FIN_ is " << has_not_sent_FIN_ << "\n";
  std::cout << "stream_is_finished_ is " << stream_is_finished_ << "\n";
  // DEBUGING

  if(unsent_msgs_.empty()) {
    // no unsent msgs buffered, see if we have to send SYN / FIN
    bool can_send_FIN = (has_not_sent_FIN_ && stream_is_finished_);
    if((has_not_sent_SYN_ || can_send_FIN) && (window_size_ > unacked_seqnos_ + unsent_seqnos_)) {
      // DEBUGING
      std::cout << "unsent_msgs_.empty() && (has_not_sent_SYN_ or has_not_sent_FIN_)\n";
      std::cout << "has_not_sent_SYN_ is " << has_not_sent_SYN_
        << ", has_not_sent_FIN_ is " << has_not_sent_FIN_ << "\n";
      // DEBUGING

      // The first msg would be the SYN message
      // Here it does not carry the payload
      TCPSenderMessage msg;
      msg.SYN = has_not_sent_SYN_;
      msg.FIN = can_send_FIN;
      msg.seqno = Wrap32::wrap(next_seqno_, isn_);
      next_seqno_ += (has_not_sent_SYN_ + can_send_FIN);
      unacked_seqnos_ += (has_not_sent_SYN_ + can_send_FIN);
      unacked_msgs_.push_back(msg);

      has_not_sent_SYN_ = false;
      has_not_sent_FIN_ = !can_send_FIN;

      // This message must be tracked in the unacked msgs buffer too
      if(!timer_.isOn()) {
        timer_.start();
      }
      return msg;
    }

    // DEBUGING
    std::cout << "unsent_msgs_.empty() && !has_not_sent_SYN_\n";
    // DEBUGING

    return std::nullopt;
  }

  // DEBUGING
  std::cout << "unsent_msgs_ is not empty, printing the unsent_msgs_ : \n";
  for(auto & msg : unsent_msgs_) {
    std::cout << msg.toString();
  }
  // DEBUGING

  // There's message in the unsent buffer, send the first one
  TCPSenderMessage msg = unsent_msgs_.front();
  // Carry SYN if SYN unsent
  if(has_not_sent_SYN_) {
    msg.SYN = true;
    has_not_sent_SYN_ = false;
  }
  uint64_t seq_length = msg.sequence_length();
  unsent_seqnos_ -= seq_length;
  unacked_seqnos_ += seq_length;
  unsent_msgs_.pop_front();
  unacked_msgs_.push_back(msg);
  if(!timer_.isOn()) {
    timer_.start();
  }
  return msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // DEBUGING
  std::cout << "[TCPSender::push] unacked_seqnos_ is " << unacked_seqnos_ << "\n";
  std::cout << "window_size_ is " << window_size_ << ", unsent_seqnos_ is " << unsent_seqnos_ << "\n";
  // DEBUGING

  std::string_view data = outbound_stream.peek();
  // int data_index = 0;
  // while((data_index < data.size()) && (unsent_seqnos_ < window_size_)) {
  //   // TODO: Emplace back
  //   // TODO: Consider the maximum size of each packet
  //   // ...
  //   unsent_msgs_.emplace_back(/* .. */);
  //   unsent_msgs_.size()
  // }
  // uint64_t data_index = 0;
  if(!data.empty()) {
    // TCPConfig::MAX_PAYLOAD_SIZE
    if(window_size_ > (unacked_seqnos_ + unsent_seqnos_)) {
      // DEBUGING
      std::cout << "(!data.empty()) and (window_size_ > (unacked_seqnos_ + unsent_seqnos_))\n";
      // DEBUGING

      std::string data_to_be_popped = std::string(data.substr(0, window_size_ - unacked_seqnos_ - unsent_seqnos_));
      outbound_stream.pop(data_to_be_popped.size());
      // NOTE: pop the reader first, then check if it's finished
      // If the reader is finished, and we have enough space in the window for a FIN bit
      // Then we carry FIN in this message
      stream_is_finished_ = outbound_stream.is_finished();
      bool FIN_flag = (has_not_sent_FIN_) && (stream_is_finished_) && (window_size_ - unacked_seqnos_ - unsent_seqnos_ >= 1);
      


      unsent_msgs_.emplace_back(Wrap32::wrap(next_seqno_, isn_), false, 
        std::string(data_to_be_popped), FIN_flag);
      has_not_sent_FIN_ = (has_not_sent_FIN_) && (!FIN_flag);
      // If we have send a FIN inside the message, set the "has_not_sent_FIN" flag to be false.
      // Therefore we would not resend it when "unsent_msgs_" is empty
      uint64_t seq_length = unsent_msgs_.back().sequence_length();
      unsent_seqnos_ += seq_length;
      // has_not_sent_SYN_ = false;
      next_seqno_ += seq_length;
    }
  } else {
    stream_is_finished_ = outbound_stream.is_finished();
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  // TODO: Figure out why
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap(next_seqno_, isn_);
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.

  // DEBUGING
  if(msg.ackno.has_value()) {
    std::cout << "[TCPSender::receive], msg is \n";
    std::cout << msg.toString();
    uint64_t ab_ackno = msg.ackno.value().unwrap(isn_, next_seqno_);
    std::cout << "absolute ackno is " << ab_ackno << "\n";
  }
  // DEBUGING

  window_size_ = msg.window_size;
  bool receive_ack = false;
  if(msg.ackno.has_value()) {
    uint64_t absolute_ackno = msg.ackno.value().unwrap(isn_, next_seqno_);
    if(absolute_ackno > next_seqno_) {
      // impossible ackno which is larger than the next seqno, ignore it
      return;
    }
    while(!unacked_msgs_.empty()) {
      auto & unacked_msg = unacked_msgs_.front();
      uint64_t seq_length = unacked_msg.sequence_length();

      // DEBUGING
      std::cout << "[TCPSender::receive] Inside the loop, (unacked_msg.seqno).unwrap(isn_, next_seqno_) + seq_length is "
        << ((unacked_msg.seqno).unwrap(isn_, next_seqno_) + seq_length)
        << ", and absolute_ackno is " << absolute_ackno << "\n";
      // DEBUGING

      if((unacked_msg.seqno).unwrap(isn_, next_seqno_) + seq_length
        > absolute_ackno) {
        break;
      }

      // DEBUGING
      std::cout << "[TCPSender::receive] pop an unacked msg and add it to the unsent_msgs : " 
      << unacked_msg.toString() << "\n";
      // DEBUGING

      unacked_seqnos_ -= seq_length;
      unacked_msgs_.pop_front();
      assert(timer_.isOn());
      receive_ack = true;
      // If the ack does not trigger any deletion of the unacked msgs buffer, 
      // we don't reset the RTO for the timer

      // DEBUGING
      std::cout << "seq_length is " << seq_length << ", unacked_seqnos_ is " << unacked_seqnos_ << std::endl;
      // DBBUGING
    }

    if(receive_ack) {
      timer_.resetRTO();
      consecutive_retransmission_num_ = 0;
      // TODO: Maybe redundance here?
      if(unacked_msgs_.empty()) {
        timer_.stop();
        // TODO: Maybe NO NEED?
      } else {
        timer_.start();
      }
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if(timer_.isOn()) {
    timer_.tick(ms_since_last_tick);
    if(timer_.hasExpired()) {
      // Retransmit
      TCPSenderMessage msg = unacked_msgs_.front();
      uint64_t seq_length = msg.sequence_length();
      unacked_seqnos_ -= seq_length;
      unsent_seqnos_ += seq_length;
      unacked_msgs_.pop_front();
      // Insert into the unsent_msgs_. I think I only need to 
      //push front, TODO: check it
      unsent_msgs_.push_front(msg);

      if(window_size_ != 0) {
        consecutive_retransmission_num_++;
        timer_.doubleRTO();
      }
      timer_.start();
    }
  }
}