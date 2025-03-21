#pragma once

#include "buffer.hh"
#include "ethernet_header.hh"
#include "parser.hh"

#include <vector>

struct EthernetFrame
{
  EthernetHeader header {};
  std::vector<Buffer> payload {};

  EthernetFrame() = default;

  EthernetFrame(const EthernetAddress & arg_src, uint16_t arg_type,
    std::vector<Buffer> && arg_payload)
    : header(arg_src, arg_type), payload(std::move(arg_payload)) {}

  EthernetFrame(const EthernetAddress & arg_dst, const EthernetAddress & arg_src, uint16_t arg_type,
    std::vector<Buffer> && arg_payload)
    : header(arg_dst, arg_src, arg_type), payload(std::move(arg_payload)) {}

  void parse( Parser& parser )
  {
    header.parse( parser );
    parser.all_remaining( payload );
  }

  void serialize( Serializer& serializer ) const
  {
    header.serialize( serializer );
    serializer.buffer( payload );
  }
};