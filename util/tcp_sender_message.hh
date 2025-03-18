#pragma once

#include "buffer.hh"
#include "wrapping_integers.hh"

#include <string>
#include <sstream>

/*
 * The TCPSenderMessage structure contains the information sent from a TCP sender to its receiver.
 *
 * It contains four fields:
 *
 * 1) The sequence number (seqno) of the beginning of the segment. If the SYN flag is set, this is the
 *    sequence number of the SYN flag. Otherwise, it's the sequence number of the beginning of the payload.
 *
 * 2) The SYN flag. If set, it means this segment is the beginning of the byte stream, and that
 *    the seqno field contains the Initial Sequence Number (ISN) -- the zero point.
 *
 * 3) The payload: a substring (possibly empty) of the byte stream.
 *
 * 4) The FIN flag. If set, it means the payload represents the ending of the byte stream.
 */

struct TCPSenderMessage
{
  Wrap32 seqno { 0 };
  bool SYN { false };
  Buffer payload {};
  bool FIN { false };

  TCPSenderMessage() {}
  TCPSenderMessage(Wrap32 arg_seqno, bool arg_SYN, std::string arg_payload_str, bool arg_FIN)
    : seqno(arg_seqno), SYN(arg_SYN), payload(arg_payload_str), FIN(arg_FIN) {}

  // How many sequence numbers does this segment use?
  size_t sequence_length() const { return SYN + payload.size() + FIN; }

  // For debuging only
  std::string toString() const {
    std::stringstream ss;
    ss << "{ seqno: " << seqno.getRawValue() << ", SYN: " << SYN << ", payload size: " 
      << payload.size() << ", FIN: " << FIN << " }\n";
    return ss.str();
  }
};

/**
 * @brief A wrapper class for TCPSenderMessage, add a "is_retransmitted" flag.
    The flag is used for deciding where to push the sent-but-not-acked messages.
    If this message is a retransmitted one, we should "push_front" to the "unacked_msgs" buffer
    Otherwise we should "push_back".
    This saves a lot of time compared to searching for the space to insert.
 */
struct TCPSenderMessageWrapper {
  TCPSenderMessage msg;
  bool is_retransmitted;

  TCPSenderMessageWrapper(TCPSenderMessage arg_msg, bool arg_is_retransmitted = false) 
    : msg(arg_msg), is_retransmitted(arg_is_retransmitted) {}

  TCPSenderMessageWrapper(Wrap32 arg_seqno, bool arg_SYN, std::string arg_payload_str, 
      bool arg_FIN, bool arg_is_retransmitted = false)
    : msg(arg_seqno, arg_SYN, arg_payload_str, arg_FIN), is_retransmitted(arg_is_retransmitted) {}

    // For debuging only
    std::string toString() const {
      std::stringstream ss;
      ss << "{ msg: " << msg.toString() << ", is_retransmitted: " << is_retransmitted << " }\n";
      return ss.str();
    }
};