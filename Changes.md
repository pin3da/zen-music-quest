Change log
==========

This doc aims to log the development of the project.


~~At this moment, you can send/receive a file as is explained at [ZMQ
reference](http://zguide.zeromq.org/page:all#Transferring-Files) with no
test.~~

It's possible to search songs and play them, once at time.
The search has no cache system implemented, so each query is made in every server.

The time complexity of each search is given by the depth of the tree, moreover the "message complexity" is linear with respect to the number of servers.
