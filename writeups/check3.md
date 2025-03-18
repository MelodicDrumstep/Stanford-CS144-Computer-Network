Checkpoint 3 Writeup
====================

So much fun in this lab.

In my implementation, I use two seperate deques, one to store the pushed-but-not-yet-sent msgs, 
the other to store the sent-but-not-yet-acked msgs. 

And it can be observed that, if we add a flag to the msgs stored 
in the "unsent_msgs" deque, we can differentiate the case, when we need to push the msg from "unsent_msgs" 
to the __front__ or __back__ of the "unacked_msgs". This trick is implemented in [tcp_sender_message.hh](../util/tcp_sender_message.hh).

And the TCP Sender contains so many special rules and must be dealt with care. The unit tests are awesome : it covers a huge amount of special cases. Please checkout the comments in my code for more details.

