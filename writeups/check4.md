Checkpoint 4 Writeup
====================

This lab is quite easy. Just follow the rules and everything will work out.

For the timeout mechanism, I use a special design : I use a queue (std::deque) to store the ARP mapping node, which contains a timestamp field. And I maintain a global timestamp. Each time "tick" is called, I check the front of the queue and invalidate some mappings. And I maintain a hashmap outside, mapping from target_ip to mapping nodes. It can be further optimized to use ring buffer and other flat designs.