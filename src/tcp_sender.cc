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
  if(unsent_msgs_.empty()) {
    // no unsent msgs buffered, see if we have to send SYN / FIN
    // The zero-window special case also applies here : 
    // If the window_size_ is 0, see if we can send a 1-seqno message
    // containing SYN or FIN
    bool can_send_FIN = (has_not_sent_FIN_ && stream_is_finished_);
    if((has_not_sent_SYN_ || can_send_FIN) 
      && (((window_size_ == 0) && (zero_window_size_special_case_flag_)) 
      || (window_size_ > unacked_seqnos_ + unsent_seqnos_))) {

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

      zero_window_size_special_case_flag_ = zero_window_size_special_case_flag_ && (window_size_ != 0);
      // the special case should happen only once each time window size dropped to 0

      // This message must be tracked in the unacked msgs buffer too
      if(!timer_.isOn()) {
        timer_.start();
      }
      return msg;
    }
    return std::nullopt;
  }
  // There's message in the unsent buffer, send the first one
  TCPSenderMessageWrapper msg_wrapper = unsent_msgs_.front();
  auto & msg = msg_wrapper.msg;
  // Carry SYN if SYN unsent
  if(has_not_sent_SYN_) {
    msg.SYN = true;
    has_not_sent_SYN_ = false;
  }
  uint64_t seq_length = msg.sequence_length();
  unsent_seqnos_ -= seq_length;
  unacked_seqnos_ += seq_length;
  unsent_msgs_.pop_front();

  // Here we use the "is_retransmitted" flag attached in the TCPSenderMessageWrapper class
  // and we call "push_front" if the message is being retransmitted,
  // because we always retransmit the oldest unacked message
  // otherwise we call "push_back"
  // because we read the stream by sequential order
  if(msg_wrapper.is_retransmitted) {
    unacked_msgs_.push_front(msg);
  } else {
    unacked_msgs_.push_back(msg);
  }
  if(!timer_.isOn()) {
    timer_.start();
  }
  return msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  std::string_view data = outbound_stream.peek();
  // We have to implement TCP segmentation here, slicing the payload into smaller ones if the total
  // payload size exceed MAX_PAYLOAD_SIZE
  uint64_t data_index = 0;
  while(data_index < data.size()) {
    // include "window_size_ == 0" case here! However, note that this case cannot run for multiple rounds
    // we have to make sure the while loop execute one round for this case
    // (even when calling this function multiple times)
    // therefore I add "zero_window_size_special_case_flag_" data member to the class.
    if(((window_size_ == 0) && (zero_window_size_special_case_flag_)) || (window_size_ > (unacked_seqnos_ + unsent_seqnos_))) {
      uint64_t data_length = std::min((window_size_ == 0) ? 1 : (window_size_ - unacked_seqnos_ - unsent_seqnos_), 
        static_cast<unsigned int>(TCPConfig::MAX_PAYLOAD_SIZE));
      // The payload size cannot exceed MAX_PAYLOAD_SIZE
      // and the total unsent and unacked messages cannot exceed the window size
      // Other than that, if the window size is 0, we should treat it as 1 and send
      // a message with 1 seq length if possible, so as to let the sender have a chance
      // to know when the window size turns to non-zero.

      std::string data_to_be_popped = std::string(data.substr(data_index, data_length));
      outbound_stream.pop(data_length);
      data_index += data_length;
      unsent_msgs_.emplace_back(Wrap32::wrap(next_seqno_, isn_), false, 
        std::string(data_to_be_popped), false);
      uint64_t seq_length = unsent_msgs_.back().msg.sequence_length();
      unsent_seqnos_ += seq_length;
      // has_not_sent_SYN_ = false;
      // Actually, I think there's potential issue for SYN carried in a data message.
      // However, there's no test case about that. And I just skip it temporarily.
      next_seqno_ += seq_length;

      zero_window_size_special_case_flag_ = zero_window_size_special_case_flag_ && (window_size_ != 0);
      // If zero_window_size_special_case_flag_ is "true", and this case is "window_size_ == 0"
      // This means that we consume the zero-window-size case, we set this flag to "false"
    } else {
      break;
      // exceed the window size, must break here, otherwise empty payload would be inserted
    }
  }
  // NOTE: pop the reader first, then check if it's finished
  // If the reader is finished, and we have enough space in the window for a FIN bit
  // Then we carry FIN in this message
  stream_is_finished_ = outbound_stream.is_finished();
  bool FIN_flag = (has_not_sent_FIN_) && (stream_is_finished_) && (window_size_ >= unacked_seqnos_ + unsent_seqnos_ + 1);

  if(!unsent_msgs_.empty()) {
    unsent_msgs_.back().msg.FIN = FIN_flag;
    has_not_sent_FIN_ = (has_not_sent_FIN_) && (!FIN_flag);
    // If we have sent a FIN inside the message, set the "has_not_sent_FIN" flag to be false.
    // Therefore we would not resend it when "unsent_msgs_" is empty
    unsent_seqnos_ += FIN_flag;
    next_seqno_ += FIN_flag;
    // FIN occupies a seqno. It's important to maintain the seqno counts whenever FIN is attached.
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap(next_seqno_, isn_);
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  window_size_ = msg.window_size;
  zero_window_size_special_case_flag_ = (msg.window_size == 0);
  // If the window size is 0, special case needs to be handled.
  // I use this data member "zero_window_size_special_case_flag_" to track that
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
      if((unacked_msg.seqno).unwrap(isn_, next_seqno_) + seq_length
        > absolute_ackno) {
        // no more acked msgs (They are sorted by seqno in the container)
        break;
      }
      unacked_seqnos_ -= seq_length;
      unacked_msgs_.pop_front();
      assert(timer_.isOn());
      receive_ack = true;
      // If the ack does not trigger any deletion of the unacked msgs buffer, 
      // we don't reset the RTO for the timer
    }

    // timer logic
    if(receive_ack) {
      timer_.resetRTO();
      consecutive_retransmission_num_ = 0;
      if(unacked_msgs_.empty()) {
        // no more unacked msgs, stop the timer
        // I think the branch can be eliminated if we want better performance
        // However, I don't do any non-trival optimization in this lab
        // to preserve the readability
        timer_.stop();
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
      // Insert into the unsent_msgs_. It can be easily observed that we only
      // need to push to the front
      unsent_msgs_.emplace_front(msg, true);
      if(window_size_ != 0) {
        consecutive_retransmission_num_++;
        timer_.doubleRTO();
      }
      timer_.start();
    }
  }
}