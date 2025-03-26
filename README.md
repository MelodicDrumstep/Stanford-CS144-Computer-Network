# CS144: Computer Network

This repository contains my implementation of the labs for Stanford CS144: Computer Networks Spring 2023. For learning reference only, please do not plagiarize.

## Layered Network Model

Through this project, I gained a deep understanding of the layered architecture of computer networks and the responsibilities of each layer.

1. Physical Layer

In this project's Lab 0: Byte Stream, we essentially implemented a simplified version of the physical layer, which provides a means to send and receive raw bytes.

2. Data Link Layer

Lab 4 of this project focuses on communication between network interfaces. Previously, I didn't fully grasp the data link layer, but now my understanding is:

   + The data link layer is responsible for communication between directly connected or switch-connected devices within the same subnet.

   + The network layer cannot replace the data link layer! The data link layer also handles collision avoidance and retransmission after collisions.

Our implementation of the network interface uses the ARP protocol, maintaining a local cache of "IP address -> MAC address" mappings. When sending an Ethernet frame, if the MAC address corresponding to the target IP is cached, the packet is sent directly. Otherwise, an ARP request is broadcast to all devices connected directly or via switches to query the MAC address for that IP. (Switches handle packet forwarding, making devices that aren't physically directly connected behave as if they were.)

3. Network Layer

Lab 5 of this project implements router logic. My understanding of the network layer is:

   + The network layer enables communication across different subnets.

   + Routers at the network layer use routing algorithms to determine the next-hop network interface.

This lab doesn't involve complex routing algorithms but focuses on implementing the routing table logic. Here, we follow the "longest prefix matching" rule to forward packets to the next-hop network interface (or the destination network interface).

4. Transport Layer

Labs 1/2/3 of this project implement the TCP protocol, including sliding windows and timeout retransmission mechanisms. My understanding is that the transport layer provides advanced data transmission features, such as the reliability, flow control, and congestion control offered by TCP.

## The whole Procedure of Receiving and Transmitted Network Packets

This issue wasn't covered in the current project, but I find it extremely interesting and would like to explore it here:

How exactly do our packets travel from the network card to kernel space and then to user space, or get sent from user space through the kernel to the network card?

Here, we'll discuss the Linux operating system and a relatively common implementation of a network card driver.

## Reference

1. [How TCP Sockets Work](https://eklitzke.org/how-tcp-sockets-work)

2. [How do network packets get to the CPU from the NIC?](https://www.reddit.com/r/compsci/comments/aj0jpx/how_do_network_packets_get_to_the_cpu_from_the_nic/?rdt=40119)

3. [How to achieve low latency with 10Gbps Ethernet](https://blog.cloudflare.com/how-to-achieve-low-latency/)

4. [What are the differences between Kernel Buffer, TCP Socket Buffer and Sliding Window](https://stackoverflow.com/questions/53691760/what-are-the-differences-between-kernel-buffer-tcp-socket-buffer-and-sliding-wi)

5. [How does buffering for TCP packets work?](https://unix.stackexchange.com/questions/645074/how-does-buffering-for-tcp-packets-work)

6. [NETWORK BUFFER](https://medium.com/@shivammishra20121999/network-buffer-af57adc893b1)

7. [How much operations of copy and of read occur in the processing of data in the stack TCP/IP?](https://stackoverflow.com/questions/28127124/how-much-operations-of-copy-and-of-read-occur-in-the-processing-of-data-in-the-s)

8. [data path (travel) of tcp data from "write" syscall downto I/O registers programming](https://stackoverflow.com/questions/2687772/data-path-travel-of-tcp-data-from-write-syscall-downto-i-o-registers-program?rq=4)

9. [Why no zero-copy networking in linux kernel?](https://stackoverflow.com/questions/22150022/why-no-zero-copy-networking-in-linux-kernel)

10. [Raw socket, Packet socket and Zero copy networking in Linux](https://yusufonlinux.blogspot.com/2010/11/data-link-access-and-zero-copy.html)

11. [Monitoring and Tuning the Linux Networking Stack: Receiving Data](https://blog.packagecloud.io/monitoring-tuning-linux-networking-stack-receiving-data/)

12. [Monitoring and Tuning the Linux Networking Stack: Sending Data](https://blog.packagecloud.io/monitoring-tuning-linux-networking-stack-sending-data/)

13. [Illustrated Guide to Monitoring and Tuning the Linux Networking Stack: Receiving Data](https://blog.packagecloud.io/illustrated-guide-monitoring-tuning-linux-networking-stack-receiving-data/)