Zen Music Quest - Basic architecture
====================================
##Introduction:
This Project contains 3 Basic models working in conjunction, providing a music listening service (A very basic one) called Zen Music Quest ( a homage to Zero MQ):

##Client: 
This guy just wants to listen to music and relies on the other guys to get his hunger for tunes satisfied (what a douche), on first instance he will connect to a Broker which will then give him another address to connect to, this is the address of a server which will do the client's bidding (such an obedient fella), when the client wants to listen to a song, he will ask the server for yet another address to connect to, which is the address of a server containing the song, which will send it directly to him.

###Socket Description
               ______
              |      |
       REQ ---->0    |
    (connect) |      |
       REQ ---->1    |
    (connect) |      |
    DEALER ---->2    |
    (connect) |______|

####Socket 0: 
a Type REQ socket which receives the data from the broker, which contains the server he must connect to.
####Socket 1: 
a Type REQ socket that will connect  and remain connected to a server which will provide search data, and the addresses to connect to in order to download songs.
####Socket 2: 
a Type DEALER socket that will connect to different servers in order to download a song from them

##Broker:
This guys is basically the manager here, he has the address of all the servers in the network, and the number of clients connected to each of them. Initially all clients connect to him to ask them for a server (might as well call this guy a pimp), he checks how many clients each server has and responds to the client with the address of the server with the least clients connected to him. Also each time a new server arises, it sends a message to the broker telling him that, and the broker adds him/her to the list.

###Socket Description

             ______
            |      |
      REP --->0  1<---REP
    (bind)  |______| (bind)

####Socket 0:
Type REP socket to which all servers connect when they arise in order to register with the broker.

####Socket 1:
Type REP socket to which clients connect in order to obtain the address to a server.

##Server:
This guy is the serious worker here, he has a limited number of other servers connected to him, and he is connected to another server in a sort of Tree Architecture, he has a number of songs in it's own hard drive, which he can serve to any clients that get his address, he can also search the tree for any song a client connected to him asks for and returns the address of the server who has it to that client, it also has a cache of the most popular songs his clients ask for and the addresses to them, in order to send them directly. As an extended functionality he can replicate a song that has been asked to him too many times to his neighbors. When a new Server Arises, he sends a message to the Broker telling him where to find him. 

###Socket Description

                 _______
                |        |
        ROUTER --->0  1<--- ROUTER
        (bind)  |        |  (bind)
                |        |
         REQ  --->2  3<---   REQ
      (connect) |________| (connect)
      
####Socket 0:
Type ROUTER socket where other servers will connect to (the ones that would be the children from a tree architecture perspective).

####Socket 1:
Type ROUTER socket to which clients will connect in order to search for  music, through this socket the server will send the address of the server that have the song requested to the client who requested it.
If the server is asked for a download, the song will be sent to the client.

####Socket 2:
Type REQ socket that will connect to another server (the one that would be the father rom a tree architecture perspective).

####Socket 3:
Type REQ socket to notify the existence of a new server.

