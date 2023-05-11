# Project 3: XFS
X500:
tian0138 Minrui Tian
wan00807 Yidan Wang
## Pledge:
No one sought out any on-line solutions

## Before compile

replce file path in line 708 in node.c to your local path

## How to compile and run
make
```
make all
make server
make client
```


### Run Server:

```
./server
```

### Run Nodes:

```
./node
nodeID //must be node1 - node5
function filename //(functions include download and find) (filename of your choice)
```


### Design:

#### Server:
The server tracks what files are in each node.

For the server, there are MAXCLIENT+1 ports. One port for pinging for each node and one port for various other requests that come from different nodes.

Since there is only one thread that handle various other request one at a time and all other thread is doing so there is no race condition at the server side.

Upon receiving download and find request, it will send back a list of nodes that contains that file.

Upon receiving a boot message, it will update its file list for the sender node and send back a port for ping/heartbeat.

Similarly, when periodically receiving a update message, it will just update its file list for the sender node and send back an acknowledge.

Upon receiving a ping message, it will respond pong to tell the node that the server is still alive.

Upon receiving a fail message, it will delete the node that is specified in the message from the file list indicating the node is down and other request would not include it.

Once it recovers, it waits live nodes' information to update its list.



#### Nodes:
The nodes handle download and find requests from user.

Each node has MAXCLIENT-1 ports reserved to receiving from other nodes so that once other nodes can not receive feedback it will know the node that it is requesting is down. And if it doesn't hear back from server in a given time, it knows the server is down right away.

Once it boot, it will read the latency file to the local data structure as well as port information and tell the server all the files it has and waiting for user and node request and heartbeating server to check its status.

Once the server is down, it will keep sending boot message to server until the server is back or user terminate the program.

The node would only know if other nodes are down when it is either trying to get load from each node or it is sending download request to each node. When there is a node down detected by the current sender node, it will notify the server to delete that dead node from its node list.

Once the user want to download a file to the local node. The node will ask the server to give it a list of nodes that have that file. The node will send getload request to nodes on the list and get latency from its local data structure. 

Then the node will sort it based on a algorithm that will be explained below to find the best node to request for that file. 

If the best one is down, it will send a notification to the server to delete it from the server side nodelist indicating that node is down and try the second best and so on until it hear back from one of the node on the list and download it to its local repo  and tell the server to update its node list or if all the nodes on the list are down then it will print to tell the user all the possible to get file from is down and waiting for other request. 

By default, we set there is 25% that the received file is corrupted.
If the file is corrupted then it will try to refetch until it get it or there is no possible node to request then it will print to notify the user.


When receiving a download request from other node, it will give back its file content. 

When receiving a getload request from other node, it will give back its load number.

Upon receiving a find request from user, it will request server to give it a list of all the live nodes have that file.

If the file doesn't exist, it will print to notify the user.

There may be potential race condition when there are mutiple node tring to download from the same node and the node itself tring to update/boot to the server. And when there are multiple task/load the node has to deal with then the load_index may face race conditon. Those situations should be prevented by the use of locks.

#### Algorithm

The algorithm is a weighted latency and load algorithm. The latency we choose are couple thousands. We want latency to play a bigger but not dominant role since user are usually not very patient to wait for the result. The load still plays a role here. Once the load is too big, other nodes should be selected. 


### Instructions

After run at the server side

```
./server
```

and at the node side

```
./node
nodeID //must be node1 - node5
```

Then nodes and server are running and heartbeating.

At the node side:
To download a file to the local:

```
download filename // (filename of your choice)
```
It will print the outcome and waiting for your input although there might be other thread printing at the back.

To find a file in the system:

```
find filename // (filename of your choice)
```
It will print a list of nodes that has that file or notify you there is no avaiable nodes for that file


### Test Cases:
For the testing you can either follow the instruction above or you can add one more arguments as input when you run it like this:

```
./node node2 download123.txt
```

Using script will terminate the file immediately which doesn't fit our design, which is nodes are keeping waiting for new request

## Case 1:
Concurrent downloading:
After running server and node1 without input type those two in two terminal simultaneously

```
./node node2 download123.txt
```

```
./node node3 download123.txt
```
There should be 123.txt in both repos of node2 and node3
## Case 2:
Server recovery;
After running server,node1 and node2, terminate the server by typing control c and run it again.Then you should see boot message from node1 and node2.
 
Try
```
download 123.txt
```

There should be a 123.txt file in node2's repo.

## Case 3:

Node recovery;
After running server,node1 and node2, terminate node1.

Try at the node2 terminal
```
download 123.txt
```
You will see a notification "client down and fail"

Then rerun node1 and 
Try at the node2 terminal:
```
download 123.txt
```
There should be a 123.txt file in node2's repo.

## Case 4
Checksum \
In ```void* download(char* filename)```, there is a code snippet adding in-consistency to the file transfer
```
if(rand()%4==0){//modify a byte
    strcpy(&token[1],"");
    printf("change value\n");
}
```
There is a 25% rate of failing the checksum check. Another download request will be send to the server. The re-request will keep going if the checksum keep failing.
Thus, if you want to make the transer consistent, comment the code above.


## Peer Selection

score = 0.5*load+0.001*latency

In finding ```123.txt``` task, we test 3 senerio that can reflect the peer selection algorithm

When ```123.txt``` only exixst on node 1:
Node 4 and node 5 send concurrently send download request, \
Node 4: 0.000248 seconds \
Node 5: 0.000241 seconds \

When ```123.txt``` exixst on node 1 and 2:
Node 4 and node 5 send concurrently send download request, \
Node 5: 0.000211 seconds \
Node 3: 0.000166 seconds \
where thy both choose node 2 as the source which has lower latency

When ```123.txt``` exixst on node 1, 2 and 3:
Node 4 and node 5 send concurrently send download request, \
Node 4: 0.000307 seconds \
Node 5: 0.000228 seconds \

Theoretically, with more cadidate of lower latency, the download will be faster. However in our experiment, the last case perform slower than expected. The possible reason is that a) latency natured in hand testing, b) larger computational cost of sorting the score of cadidates. This should perform better when the system scaled up where the computational cost differences are smaller.