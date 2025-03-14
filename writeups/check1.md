Checkpoint 1 Writeup
====================

For Reassembler, I tried two implementations:

1. Use list to store the string segments. 

This is a naive idea and relative easy to implement. Actually I just use std::list for simplicity. (If I want better performance
 in this version I would convert it to be memory pooling + instrusive linked list). See git commit node "ec5b57c381c3d406e12a63ac51e27097ed1afaaf" for this implementation.

Actually the performance is not bad (2.08 Gbit/s). However, a lot of memory allocation and deallocation overhead lies here due to
 std::string and std::list. And I can use a flat array design to avoid that (and use a bitmap to accelerate searching).

2. Use flat array and bitmap.

