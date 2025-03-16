Checkpoint 2 Writeup
====================

The wrapping integers part is the most interesting in this lab:

I firstly write down some equations and try to do some modular magic. And soon I become confused. Later on I took reference to some ideas online and fix it.

The basic idea is to project the checkpoint into the uint32_t area, compute the difference, and project back. And one special case must be taken into consideration.

The TCP Receiver part is super easy.