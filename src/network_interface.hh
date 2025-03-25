#pragma once

#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

#include <iostream>
#include <list>
#include <optional>
#include <queue>
#include <unordered_map>
#include <utility>
#include <deque>

// A "network interface" that connects IP (the internet layer, or network layer)
// with Ethernet (the network access layer, or link layer).

// This module is the lowest layer of a TCP/IP stack
// (connecting IP with the lower-layer network protocol,
// e.g. Ethernet). But the same module is also used repeatedly
// as part of a router: a router generally has many network
// interfaces, and the router's job is to route Internet datagrams
// between the different interfaces.

// The network interface translates datagrams (coming from the
// "customer," e.g. a TCP/IP stack or router) into Ethernet
// frames. To fill in the Ethernet destination address, it looks up
// the Ethernet address of the next IP hop of each datagram, making
// requests with the [Address Resolution Protocol](\ref rfc::rfc826).
// In the opposite direction, the network interface accepts Ethernet
// frames, checks if they are intended for it, and if so, processes
// the the payload depending on its type. If it's an IPv4 datagram,
// the network interface passes it up the stack. If it's an ARP
// request or reply, the network interface processes the frame
// and learns or replies as necessary.
class NetworkInterface
{
protected:
  // Ethernet (known as hardware, network-access, or link-layer) address of the interface
  EthernetAddress ethernet_address_;

  // IP (known as Internet-layer or network-layer) address of the interface
  Address ip_address_;
  uint32_t ipv4_;

  std::deque<EthernetFrame> eth_frames_to_be_sent_ {};
  // frames that will be sent by "maybe_send" function

  struct ArpMappingNode {
    uint32_t ipv4;
    // store the ip here is necessary, as it allows us to find the corresponding
    // arp table entry when this mapping node expires
    // And we can set the arp table entry 's arp node ptr to nullptr
    // to indicating expiration
    EthernetAddress eth_addr;
    size_t timestamp;

    ArpMappingNode(uint32_t ip, const EthernetAddress& eth, size_t ts)
      : ipv4(ip), eth_addr(eth), timestamp(ts)
    {}
  };

  using ArpMappingQueue = std::deque<ArpMappingNode>;
  ArpMappingQueue arp_mapping_queue_ {};
  // sorted by timestamp

  struct ArpRequestTimeNode {
    uint32_t ipv4;
    size_t timestamp;

    ArpRequestTimeNode(uint32_t ip, size_t ts)
      : ipv4(ip), timestamp(ts)
    {}
  };

  using ArpRequestTimeQueue = std::deque<ArpRequestTimeNode>;
  ArpRequestTimeQueue arp_request_time_queue_ {};
  // sorted by timestamp

  struct ArpTableEntry {
    ArpMappingNode * arp_mapping_node_ptr = nullptr;
    // If this node is expired, arp_mapping_node_ptr will be assigned to nullptr
    // I should have design it to be a std::shared_ptr
    // However, that adds a layer of indirection to std::deque
    // And here the logic is simple. It's not difficult to get it right.
    ArpRequestTimeNode * arp_request_ptr = nullptr;
    std::deque<InternetDatagram> pending_ip_datagrams {};
  };

  std::unordered_map<uint32_t, ArpTableEntry> arp_table_ {};
  // arp_table is a "ip -> arp queue iterator" mapping

  size_t timestamp_ = 0;
  constexpr static size_t ArpTableEntryTimeoutMs = 30'000;
  constexpr static size_t ArpRequestWaitingMs = 5'000;

public:
  // Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer)
  // addresses
  NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address );

  // Access queue of Ethernet frames awaiting transmission
  std::optional<EthernetFrame> maybe_send();

  // Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination
  // address). Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address
  // for the next hop.
  // ("Sending" is accomplished by making sure maybe_send() will release the frame when next called,
  // but please consider the frame sent as soon as it is generated.)
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // Receives an Ethernet frame and responds appropriately.
  // If type is IPv4, returns the datagram.
  // If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // If type is ARP reply, learn a mapping from the "sender" fields.
  std::optional<InternetDatagram> recv_frame( const EthernetFrame& frame );

  // Called periodically when time elapses
  void tick( size_t ms_since_last_tick );

  const EthernetAddress & getMacAddress() {
    return ethernet_address_;
  }

  // FOR DEBUGGING ONLY
  std::string toString() const {
    std::string ret = "IP: " + ip_address_.to_string() + ", MAC: ";
    for(auto c : ethernet_address_) {
      ret += (std::to_string(c) + " ");
    }
    return ret;
  }
};
