# p2p_network

P2P: Peer-to-peer (P2P) networks are a type of decentralized network in which all
nodes (also called peers) can communicate and share resources with each other
without the need for a central server. Socket programming is a way of creating
networked applications in which two or more nodes can communicate with each other
over a network using sockets. Combining P2P network and socket programming can
create a powerful distributed system that can be used for various applications.
In peer-to-peer architecture all nodes (peers) function simultaneously as both clients
and servers.

Advantages: Availability, Inexpensive, Scalability, Performance

Disadvantage: Security, Regulation, Performance

Examples: BitTorrent (file sharing), Tor (anonymous internet browsing), and
Bitcoin (a decentralized transaction ledger).

Assignment Task:

Assume there is one music file called. Music.txt which was divided in chunks and
chunks are distributed to the P2P network. You need to implement a method which can
provide functionality to the particular peer to connect with the network, search particular
chunks from the network and download chunk files from the network.

Steps to create P2P network:

Step 1. Network Join/Leave Architecture.

You need to consider some default node list is given which was maintained by the
anchor node. An anchor node is nothing but well known server1 and server2 which we
used in previous labs.
When a new application starts. A peer first connects to one of the anchor nodes, and
sends requests to the anchor node. In response anchor nodes return a list of eligible
P2P nodes.
Peer nodes connect to one of these nodes. For connection with nodes you need IP of
those nodes, and that thing is available in the list which was returned by the anchor
node.

Step 2. Resource storage and distribution.

Now, when the peer node wants to download a file, let’s say named “music.txt” which is
divided into chunks of a specific size and distributed over the network, the peer sends a
request to get the file to the anchor node. In response to that the anchor node sends a
manifest file for the “music.txt” file. The manifest file contains information about the
chunks and which peers in the network have the chunk.
It’s possible that more than one peer has a chunk, So, whichever peer has a chunk of a
file, that peer’s IP address is associated with the chunk in the manifest file.

Now, after getting the manifest file, the chunks needs to be downloaded from the
respective IP address peers. When a chunk is downloaded it needs to be acknowledged
to the anchor node so that the anchor node can be updated and the peer who asked for
the specific node, its ip address can be appended in the manifest file for that chunk.
So, now when another peer wants to get that chunk, it can be obtained from this peer as
well.
In P2P network all the peers act like client as well as server.


Step 3. Download a file/chunk:

From the second step (search a file) you got an idea about manifest file which will
contain all details about different chunks like name,size,i/p addresses etc..Now using
different process/connection using client-server socket programming,you are suppose
to download a file or chunk that could be present at different nodes of your P2P
network.
The file download should work in such a manner that it it get downloaded as a one
single file in your system.For that you’d be require to concatenate the chunks in different
process.For example, music.txt file will have chunks life music1.txt,music2.txt,..etc.They
might be present at anchor node,peer nodes etc..So you will be required to download it
as one combined music.txt file.

IPC:

As you all have already studied IPC in system programming. You need to use conecpt
of IPC to manage 3 request encountered by anchor node.

First one is getting a connection request. Second one is to update the manifest file.
Third one is file update/download.
All three tasks will be done in a separate process. Using IPC you can create a new
process for each task. And assign tasks to particular processes.
