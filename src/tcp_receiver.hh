#pragma once

#include <optional>

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPReceiver
{
public:
  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */

  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

  /* The TCPReceiver sends TCPReceiverMessages back to the TCPSender. */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;

private:
  std::optional<Wrap32> initial_seq_num_ = std::nullopt;
  uint64_t first_unassembled_index_ = 0;
  bool meet_FIN_ = false;
  bool is_finished_ = false;
};
