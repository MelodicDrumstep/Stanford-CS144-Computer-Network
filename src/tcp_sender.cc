#include <random>
#include <deque>

#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), 
    initial_RTO_ms_( initial_RTO_ms ),
    next_seqno_(Wrap32::wrap(0, isn_)),
    timer_(intial_RTO_ms)
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return {};
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return {};
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if(unsent_msgs_.empty()) {
    return std::nullopt;
  }
  TCPSenderMessage msg = unsent_msgs_.front();
  unsent_seqnos_ -= msg.sequence_length();
  unsent_msgs_.pop_front();
  unacked_msgs_.push_back(msg);
  // TODO: Timer logic
  return msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  std::string_view data = outbound_stream.peak();
  int data_index = 0;
  // while((data_index < data.size()) && (unsent_seqnos_ < window_size_)) {
  //   // TODO: Emplace back
  //   // TODO: Consider the maximum size of each packet
  //   // ...
  //   unsent_msgs_.emplace_back(/* .. */);
  //   unsent_msgs_.size()
  // }
  unsent_msgs_.emplace_back(next_seqno_, has_to_sent_SYN_, std::string(data.substr(0, window_size_ - unsent_seqnos_)), false);
  has_to_sent_SYN_ = false;
  // TODO: Deal with FIN
  next_seqno_ = next_seqno_ + unsent_msgs_.back().sequence_length();
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  // TODO: Figure out why
  TCPSenderMessage msg;
  msg.seqno = next_seqno_;
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  window_size_ = msg.window_size;
  if(msg.ackno.has_value()) {
    while(!unacked_msgs_.empty()) {
      auto & unacked_msg = unacked_msgs_.front();
      if((unacked_msg.seqno + unacked_msg.sequence_length()).unwrap(isn_, checkpoint_)
        > msg.ackno.value().unwrap(isn_, checkpoint_)) {
        break;
      }
      unacked_msgs_.pop_front();
      // TODO: Timer logic
    }
  }

}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if(timer_.isOn()) {
    timer_.tick();
    if(timer_.hasExpired()) {
      // TODO:...
    }
  }
}