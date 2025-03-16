#include "tcp_receiver.hh"

#include <limits>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if(message.SYN) {
    initial_seq_num_ = message.seqno;
  }
  if(!initial_seq_num_.has_value()) {
    return;
  }
  is_finished_ = message.FIN;
  reassembler.insert(message.seqno.unwrap(initial_seq_num_.value(), reassembler.getFirstUnassembledIndex()) - 1, 
    message.payload.release(), message.FIN, inbound_stream);
  first_unassembled_index_ = reassembler.getFirstUnassembledIndex();
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage ret;
  if(initial_seq_num_.has_value()) {
    ret.ackno = Wrap32::wrap(first_unassembled_index_ + 1 + is_finished_, initial_seq_num_.value());
  }
  ret.window_size = (inbound_stream.available_capacity() >= static_cast<uint64_t>(UINT16_MAX))
     ? UINT16_MAX : inbound_stream.available_capacity();
  return ret;
}
