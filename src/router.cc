#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  router_table_.reserve(interface_num + 1);
  router_table_.emplace(router_table_.begin() + interface_num, route_prefix, prefix_length, next_hop);
}

void Router::route() 
{
  // Naive way
  for(size_t i = 0; i < interfaces_.size(); i++) {
    auto & interface = interfaces_[i];
    auto datagram_opt = interface.maybe_receive();
    if(!datagram_opt.has_value()) {
      continue;
    }
    auto & datagram = datagram_opt.value();
    auto & header = datagram.header;
    uint32_t dst_ipv4 = header.dst;
    uint8_t max_matched_len = 0;
    size_t max_matched_index = 0;
    for(size_t j = 0; j < router_table_.size(); j++) {
      auto & router_table_entry = router_table_[j];
      uint32_t route_prefix = router_table_entry.route_prefix;
      uint8_t prefix_length = router_table_entry.prefix_length;
      if(i == j) {
        // Don't send it to itself
        continue;
      }
      if(ipPrefixMatched(dst_ipv4, route_prefix, prefix_length) && (prefix_length > max_matched_len)) {
        max_matched_len = prefix_length;
        max_matched_index = j;
      }
    }
    if(max_matched_len != 0) {
      if(header.ttl == 1) {
        // TTL will drop to 0
        continue;
      }
      header.ttl--;
      auto & router_table_entry_max_matched = router_table_[max_matched_index];
      auto & matched_interface = interfaces_[max_matched_index];
      auto & next_hop = router_table_entry_max_matched.next_hop;
      if(next_hop.has_value()) {
        matched_interface.send_datagram(datagram, next_hop.value());
      } else {
        EthernetFrame ether_frame;
        parse(ether_frame, datagram.payload);
        matched_interface.recv_frame(ether_frame);
      }
    }
  }
}

bool ipPrefixMatched(uint32_t dst_ip, uint32_t route_prefix_ip, uint8_t prefix_length) {
  if(prefix_length == 0) {
    return true;
  }
  uint32_t mask = 0xFFFFFFFF << (32 - prefix_length);
  return (dst_ip & mask) == (route_prefix_ip & mask);
}