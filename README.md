zen-music-quest
===============


Distributed music service, built as a programming assignment in client-server architecture class.

Mainly we will use [ZMQ](http://zeromq.org/) to connect things, and [zmqpp](https://github.com/zeromq/zmqpp) to make it easier.

## Dependencies
- [Install ZMQ](http://zeromq.org/intro:get-the-software)
- [Install ZMQPP](https://github.com/zeromq/zmqpp)
- [libuud](http://linux.die.net/man/3/libuuid)


        aptitude install uuid-dev uuid-runtime uuid
- [libsfml](http://www.sfml-dev.org/) (2.0 or higher)
 
        aptitude install libsfml-dev 
  
## Compile

    make

## Run
You can run the app using the start script provided here

    ./start.sh num_servers

It will create a binary tree of servers with depth of log2(num_servers)

You can also run the app manually and create your own topology:

    ./broker
    ./server ip id parent_ip

If you want one or several servers without parent just set the parent_ip = "tcp://localhost:4444"

___________
Manuel Pineda - Carlos González.
