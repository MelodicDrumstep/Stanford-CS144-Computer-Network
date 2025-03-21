#include <limits>

#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if(message.SYN) {
    initial_seq_num_ = message.seqno;
    message.seqno = message.seqno + 1;
    // SYN received. Set the ISN, and increment the seqno(because the same message can contain payload)
  }
  if(!initial_seq_num_.has_value()) {
    // not synchronized, directly return
    return;
  }
  meet_FIN_ = meet_FIN_ || message.FIN;
  reassembler.insert(message.seqno.unwrap(initial_seq_num_.value(), reassembler.getFirstUnassembledIndex()) - 1, 
    message.payload.release(), message.FIN, inbound_stream);
  // subtract 1 from the unwrapped stream index here, because SYN makes it increment 1
  is_finished_ = is_finished_ || (meet_FIN_ && inbound_stream.is_closed());
  // is_finished_ data member is used to determine if we need to increment 1 when sending ACK
  // because of FIN.
  // However, we cannot set "is_finished_" once meeting FIN : We have to wait until the buffer is empty
  first_unassembled_index_ = reassembler.getFirstUnassembledIndex();
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage ret;
  if(initial_seq_num_.has_value()) {
    ret.ackno = Wrap32::wrap(first_unassembled_index_ + 1 + is_finished_, initial_seq_num_.value());
    // the first "+1" is introduced by SYN
    // and "+ is_finished_" will be introduced by FIN
  }
  ret.window_size = std::min(inbound_stream.available_capacity(), static_cast<uint64_t>(UINT16_MAX));
  // if the buffer size is larger than UINT16_MAX, do not overflow, set the window size to UINT16_MAX
  return ret;
}
