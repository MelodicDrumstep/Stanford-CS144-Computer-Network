#pragma once

#include <cassert>
#include <iostream>

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class Timer {
  public:
    Timer(uint64_t RTO_ms) : initial_RTO_ms_(RTO_ms), RTO_ms_(RTO_ms), timeout_ms_(RTO_ms) {}
  
    void tick( uint64_t ms_since_last_tick ) {
      timeout_ms_ = (timeout_ms_ <= ms_since_last_tick) ? 0 : timeout_ms_ - ms_since_last_tick;
    }

    bool isOn() const {
      return is_on_;
    }

    bool hasExpired() const {
      assert(is_on_);
      return timeout_ms_ == 0;
    }

    void doubleRTO() {
      assert(is_on_);
      RTO_ms_ *= 2;
    }

    void resetRTO() {
      assert(is_on_);
      RTO_ms_ = initial_RTO_ms_;
    }

    void start() {
      is_on_ = true;
      timeout_ms_ = RTO_ms_;
    }

    void stop() {
      is_on_ = false;
    }

  private:
    bool is_on_{false};
    uint64_t initial_RTO_ms_;
    uint64_t RTO_ms_;
    uint64_t timeout_ms_;
};

/**
 * @brief TCPSender class
  It maintains the unsent msgs buffer and unacked msgs buffer.
  unsent msgs are msgs read from the buffer and not yet sent. Once it has been sent by
  "maybe_send", it will be moved to "unacked msgs buffer". And once the timer expired,
  we move the first msgs in the unacked msgs buffer to the front of the unsent msgs buffer
  to do retransmission.
 */
class TCPSender
{
private:
  Wrap32 isn_;
  bool has_not_sent_SYN_ = true;
  bool has_not_sent_FIN_ = true;
  bool stream_is_finished_ = false;
  bool zero_window_size_special_case_flag_ = false;
  std::deque<TCPSenderMessageWrapper> unsent_msgs_ {};
  std::deque<TCPSenderMessage> unacked_msgs_ {};
  uint64_t next_seqno_ = 0; // store the absolute seqno here. It will also serve as the checkpoint when unwrapping ack
  uint32_t unsent_seqnos_ = 0;
  uint32_t unacked_seqnos_ = 0;
  uint16_t window_size_ = 1;
  uint32_t consecutive_retransmission_num_ = 0;
  Timer timer_;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?

private:
  
};