#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t ip = next_htop.ipv4_numeric();
  auto & arp_table_entry = arp_table_[ip];
  auto arp_mapping_node_ptr = arp_table_entry.arp_node_ptr;
  if(!arp_mapping_node_ptr) {
    // we do not remember this arp mapping, send a ARP request
    // timer logic here
    // TODO: 
    

    arp_table_entry.pending_ip_datagrams.push_back(dgram);
  } else {
    // We have the arp mapping, send it immediately
    EthernetAddress dst_eth_addr = arp_mapping_node_ptr -> eth_addr;
    eth_frames_to_be_sent_.emplace_back(dst_eth_addr, ethernet_address_, EthernetHeader::TYPE_IPv4, serialize(dgram));
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  auto & header = frame.header;
  if(header.type == EthernetHeader::TYPE_IPv4) {
    // Ip packet
    if(dst == ethernet_address_) {
      InternetDatagram ret;
      parse(ret, frame.payload);
      return ret;
    } else {
      // The packet is for this network interface, ignore it
      return std::nullopt;
    }
  } else if(header.type == EthernetHeader::TYPE_ARP) {
    // Arp message



  } else {
    throw std::runtime_error("[NetworkInterface::recv_frame] Invalid Ethernet Type");
  }
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  timestamp_ += ms_since_last_tick;
  while(!arp_queue_.empty()) {
    auto & arp_node = arp_queue.front();
    if(timestamp_ < arp_node.timestamp + TimeoutMs) {
      // the arp queue is sorted by timestamp. Therefore if we
      // find out the front node is not expired, we can skip checking the rest
      break;
    }
    int32_t ip = arp_node.ipv4;
    arp_table_[ip].arp_node_ptr = nullptr;
    // Set it to be "expired"
    arp_queue_.pop_front();
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(eth_frames_to_be_sent_.empty()) {
    return std::nullopt;
  }
  EthernetFrame ret = eth_frames_to_be_sent_.front();
  eth_frames_to_be_sent_.pop_front();
  return ret; 
  // Will be RVO optimized
}
