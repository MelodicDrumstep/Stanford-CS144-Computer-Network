#include "router.hh"

#include <iostream>
#include <limits>
#include <cassert>
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
  // Just maintain a 2D vector to store the "route_prefix, prefix_length, next_hop"
  // and iterate it in "route" function
  router_table_.resize(interface_num + 1);
  router_table_[interface_num].emplace_back(route_prefix, prefix_length, next_hop);
  // cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
  //      << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
  //      << " on interface " << interface_num << ", which is " << interfaces_[interface_num].toString() << "\n";
}

void Router::route() 
{
  // Naive way, enumerating the 2D vector
  for(size_t i = 0; i < interfaces_.size(); i++) {
    auto & interface = interfaces_[i];
    auto datagram_opt = interface.maybe_receive();
    if(!datagram_opt.has_value()) {
      // no datagram from this interface
      continue;
    }
    auto & datagram = datagram_opt.value();
    auto & header = datagram.header;
    uint32_t dst_ipv4 = header.dst;

    // set the default value for these variables, and start searching
    uint8_t max_matched_len = 0;
    size_t max_matched_index = std::numeric_limits<size_t>::max();
    std::optional<Address> matched_next_hop = std::nullopt;

    for(size_t j = 0; j < router_table_.size(); j++) {
      // Enumerate the router table
      auto & per_interface_router_table = router_table_[j];
      for(auto & router_table_entry : per_interface_router_table) {
        uint32_t route_prefix = router_table_entry.route_prefix;
        uint8_t prefix_length = router_table_entry.prefix_length;
        if(ipPrefixMatched(dst_ipv4, route_prefix, prefix_length) && (prefix_length >= max_matched_len)) {
          // prefix matched, renew the data if prefix_length is larger
          max_matched_len = prefix_length;
          max_matched_index = j;
          matched_next_hop = router_table_entry.next_hop;
        }
      }
    }

    if(max_matched_index != std::numeric_limits<size_t>::max()) {
      // We match some interface
      if((header.ttl == 1) || (header.ttl == 0)) {
        // TTL will drop to 0 or is already 0
        continue;
      }
      header.ttl--;
      auto & matched_interface = interfaces_[max_matched_index];
      if(matched_next_hop.has_value()) {
        // forward the datagram, and set the address to be the next hop ip
        matched_interface.send_datagram(datagram, matched_next_hop.value());
      } else {
        // direct forwarding
        // forward the datagram, and set the address to be the dst ip
        matched_interface.send_datagram(datagram, Address::from_ipv4_numeric(dst_ipv4));
      }
    }
  }
}

bool ipPrefixMatched(uint32_t dst_ip, uint32_t route_prefix_ip, uint8_t prefix_length) {
  if(prefix_length == 0) {
    // cannot left shift 32 bits for uint32_t! (Undefined-Behaviour)
    return true;
  }
  // use this mask to compare the first "prefix_length" bits
  uint32_t mask = 0xFFFFFFFF << (32 - prefix_length);
  return (dst_ip & mask) == (route_prefix_ip & mask);
}