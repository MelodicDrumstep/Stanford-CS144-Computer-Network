#pragma once

#include "network_interface.hh"

#include <optional>
#include <queue>
#include <iostream>

// A wrapper for NetworkInterface that makes the host-side
// interface asynchronous: instead of returning received datagrams
// immediately (from the `recv_frame` method), it stores them for
// later retrieval. Otherwise, behaves identically to the underlying
// implementation of NetworkInterface.
class AsyncNetworkInterface : public NetworkInterface
{
  std::queue<InternetDatagram> datagrams_in_ {};
  size_t direct_datagrams_count_ = 0;
  // This variable is used to indicate the number of just-received datagrams,
  // to avoid pulling them out in the same "route" function

public:
  using NetworkInterface::NetworkInterface;

  // Construct from a NetworkInterface
  explicit AsyncNetworkInterface( NetworkInterface&& interface ) : NetworkInterface( interface ) {}

  // \brief Receives and Ethernet frame and responds appropriately.

  // - If type is IPv4, pushes to the `datagrams_out` queue for later retrieval by the owner.
  // - If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // - If type is ARP reply, learn a mapping from the "target" fields.
  //
  // \param[in] frame the incoming Ethernet frame
  void recv_frame( const EthernetFrame& frame )
  {
    auto optional_dgram = NetworkInterface::recv_frame( frame );
    if ( optional_dgram.has_value() ) {
      datagrams_in_.push( std::move( optional_dgram.value() ) );

      // TODO: Delete this after testing
      std::cout << "[AsyncNetworkInterface::recv_frame] this interface is " << toString() 
        << ", optional_dgram has value, the ip header is \n"
        << datagrams_in_.back().header.to_string() << "\n";
      // DEBUGING
    } 
    // TODO; Delete this after testing
    else {
      std::cout << "[AsyncNetworkInterface::recv_frame] optional_dgram does not have value\n";
    }
  };

  // Access queue of Internet datagrams that have been received
  std::optional<InternetDatagram> maybe_receive()
  {
    if ( datagrams_in_.empty() ) {
      return {};
    }
    // TODO: Delete this after testing
    std::cout << "[AsyncNetworkInterface::maybe_receive] datagrams_in_.size() is "
      << datagrams_in_.size() << "\n";
    // DEBUGING

    InternetDatagram datagram = std::move( datagrams_in_.front() );
    // TODO: Delete this after testing
    std::cout << "[AsyncNetworkInterface::maybe_receive] datagram.header is "
      << datagram.header.to_string() << "\n";
    // DEBUGING
    
    datagrams_in_.pop();
    if(direct_datagrams_count_ > 0) {
      // TODO: Delete this after testing
      std::cout << "[AsyncNetworkInterface::maybe_receive] direct_datagrams_count_ will decrement." 
      " And before decrementing, it was " << direct_datagrams_count_ << "\n";
      // DEBUGING

      direct_datagrams_count_--;
    }
    return datagram;
  }

  // This is a specialized function wrapper that will be called in "route" function
  //
  std::optional<InternetDatagram> maybe_receive_route_specialized() {
    if(direct_datagrams_count_ == 0) {
      return maybe_receive();
    }
    return std::nullopt;
  }

  void incrementDirectDatagramsCount() {
    direct_datagrams_count_++;
  }
};

// A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
  // The router's collection of network interfaces
  std::vector<AsyncNetworkInterface> interfaces_ {};

  struct RouterTableEntry {
    uint32_t route_prefix;
    uint8_t prefix_length;
    std::optional<Address> next_hop;

    RouterTableEntry(uint32_t prefix = 0, uint8_t length = 0, std::optional<Address> hop = std::nullopt)
      : route_prefix(prefix), prefix_length(length), next_hop(hop)
    {}
    RouterTableEntry& operator=(const RouterTableEntry& other) {
      route_prefix = other.route_prefix;
      prefix_length = other.prefix_length;
      next_hop = other.next_hop;
      return *this;
    }
  };
  std::vector<std::vector<RouterTableEntry>> router_table_ {};

public:
  // Add an interface to the router
  // interface: an already-constructed network interface
  // returns the index of the interface after it has been added to the router
  size_t add_interface( AsyncNetworkInterface&& interface )
  {
    // TODO: Delete this after testing
    std::cout << "[Router::add_interface] add the interface " << interface.toString() << "\n";
    // DEBUGING
    
    interfaces_.push_back( std::move( interface ) );
    return interfaces_.size() - 1;
  }

  // Access an interface by index 
  AsyncNetworkInterface& interface( size_t N ) { return interfaces_.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces. For each interface, use the
  // maybe_receive() method to consume every incoming datagram and
  // send it on one of interfaces to the correct next hop. The router
  // chooses the outbound interface and next-hop as specified by the
  // route with the longest prefix_length that matches the datagram's
  // destination address.
  void route();
};

bool ipPrefixMatched(uint32_t dst_ip, uint32_t route_prefix_ip, uint8_t prefix_length);