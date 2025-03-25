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
  router_table_.resize(interface_num + 1);
  router_table_[interface_num].emplace_back(route_prefix, prefix_length, next_hop);
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << ", which is " << interfaces_[interface_num].toString() << "\n";
}

void Router::route() 
{
  // Naive way
  // TODO: Delete this after testing
  // static uint32_t cnt = 0;
  bool output_flag = true;
  // cnt++;
  if(output_flag) {
    std::cout << "[Router::route]\n";
  }
  // DEBUGING

  for(size_t i = 0; i < interfaces_.size(); i++) {
    auto & interface = interfaces_[i];
    // TODO: Delete this after testing
    if(output_flag) {
      std::cout << "[Router::route] i = " << i << ", interface is " << interface.toString() << "\n";
    }
    // DEBUGING

    auto datagram_opt = interface.maybe_receive();
    if(!datagram_opt.has_value()) {
      // no datagram from this interface
      continue;
    }
    auto & datagram = datagram_opt.value();
    auto & header = datagram.header;
    uint32_t dst_ipv4 = header.dst;
    uint8_t max_matched_len = 0;
    size_t max_matched_index = std::numeric_limits<size_t>::max();
    std::optional<Address> matched_next_hop = std::nullopt;

    // TODO: Delete this after testing
    if(output_flag) { 
      std::cout << "[Router::route] datagram header is " << datagram.header.to_string() << "\n";
    }
    // DEBUGING

    for(size_t j = 0; j < router_table_.size(); j++) {
      auto & per_interface_router_table = router_table_[j];
      for(auto & router_table_entry : per_interface_router_table) {
        uint32_t route_prefix = router_table_entry.route_prefix;
        uint8_t prefix_length = router_table_entry.prefix_length;

        // TODO: Debugging
        if(output_flag) {
          std::cout << "iterating throught the router table for interface " << j << ", and the item is :\n"
            << "route_prefix is " << Address::from_ipv4_numeric(route_prefix).to_string() 
            << ", prefix_length is " <<  static_cast<int32_t>(prefix_length) 
            << ", dst_ip is " << Address::from_ipv4_numeric(dst_ipv4).to_string() << "\n";         
        }
        // DEBUGING

        if(ipPrefixMatched(dst_ipv4, route_prefix, prefix_length) && (prefix_length >= max_matched_len)) {
          // TODO: Delete this after testing
          if(output_flag) {
            std::cout << "[Router::route] prefix matched, interface is " << interfaces_[j].toString() << "\n";
          }
          // DEBUGING

          max_matched_len = prefix_length;
          max_matched_index = j;
          matched_next_hop = router_table_entry.next_hop;
        }
      }
    }

    if(max_matched_index != std::numeric_limits<size_t>::max()) {
      // TODO: Delete this after testing
      if(output_flag) {
        std::cout << "[Router::route] max_matched_len is " << static_cast<int32_t>(max_matched_len) << "\n";
        std::cout << "matched interface is " << interfaces_[max_matched_index].toString() << "\n";
      }
      // DEBUGING

      if((header.ttl == 1) || (header.ttl == 0)) {
        // TTL will drop to 0
        // TODO: Delete this after testing
        if(output_flag) {
          std::cout << "[Router::route] before decrementing, header.ttl is " << header.ttl << "\n";
        }
        // DEBUGING

        continue;
      }
      header.ttl--;
      auto & matched_interface = interfaces_[max_matched_index];
      if(matched_next_hop.has_value()) {
        // TODO: Delete this after testing
        if(output_flag) {
          std::cout << "[Router::route] matched_next_hop.has_value(), forward the datagram " << "\n";
        }
        // DEBUGING

        matched_interface.send_datagram(datagram, matched_next_hop.value());
      } else {
        // TODO: Delete this after testing
        if(output_flag) {
          std::cout << "[Router::route] !matched_next_hop.has_value(), let the interface send the datagram directly" << "\n";
        }
        // DEBUGING

        matched_interface.send_datagram(datagram, Address::from_ipv4_numeric(dst_ipv4));
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