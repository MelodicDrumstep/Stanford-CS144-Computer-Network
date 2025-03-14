Checkpoint 0 Writeup
====================

For ByteStream, I tried two implementations:

1. Using std::deque. It's really easy but performance is bad (due to "peek" function constraint).

2. Using flat std::vector and use memcpy whenever I can. The performance is better. (0.42 ~ 0.62 Gbit/s)

I think the "peek" interface makes it harder for better performance. It requires returning a std::string_view, 
 which makes it really hard to avoid copy in ring buffer.
 I would have design a ring buffer with some simple protocol and the corresponding interface in real life.

At last, fun project!