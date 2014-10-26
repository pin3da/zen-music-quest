Zen Music Quest - Basic architecture
====================================
##Introduction:
This Project contains 3 Basic models working in conjunction, providing a music listening service (A very basic one) called Zen Music Quest ( a homage to Zero MQ):

##Client: 
This guy just wants to listen to music and relies on the other guys to get his hunger for tune satisfied (what a douche), on first instance he will connect to a Broker which will then give him another address to connect to, this is the address of a server which will do the client's bidding (such an obedient fella), when the client wants to listen to a song, he will ask the server for yet another address to connect to, which is the address of a server containing the song, which will send it directly to him.

               ______
              |      |
       REQ ---->0    |
    (connect) |      |
       REQ ---->1    |
    (connect) |      |
       REQ ---->2    |
    (connect) |______|



##Broker:
This guys is basically the manager here, he has the address of all the servers in the network, and the number of clients connected to each of them. Initially all clients connect to him to ask them for a server (might as well call this guy a pimp), he checks how many clients each server has and responds to the client with the address of the server with the least clients connected to him. Also each time a new server arises, it sends a message to the broker telling him that, and the broker adds him/her to the list.

             ______
            |      |
      RES --->0  1<---RES
    (bind)  |______| (bind)

##Server:
This guy is the serious worker here, he has a limited number of other servers connected to him, and he is connected to another server in a sort of Tree Architecture, he has a number of songs in it's own hard drive, which he can serve to any clients that get his address, he can also search the tree for any song a client connected to him asks for and returns the address of the server who has it to that client, it also has a cache of the most popular songs his clients ask for and the addresses to them, in order to send them directly. As an extended functionality he can replicate a song that has been asked to him too many times to his neighbors. When a new Server Arises, he sends a message to the Broker telling him where to find him. 

                 _______
                |        |
        ROUTER --->0  1<--- ROUTER
        (bind)  |        |  (bind)
                |        |
        ROUTER --->2  3<---  REQ
      (connect) |________| (connect)
