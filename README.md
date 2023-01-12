# FireFighter
Fire Fighters mesh network monitoring a minimalistic app that can be implememnted on IoT device for First Responders personals to insure there safty.

### Contents: 
server, node, filesManager 

## setup:
To compile write make in the command line after cd inside the application folder, you can use make clear command to deleted the program and the txt files that is created after running the program, ( this will also delete the log.txt file )
### Node (Client)
The node application can run in several modes depending on the program arguments. 
```sh
./node -p <myPort> -n <nodeNumber> -f <configuration file> Optional -r <depleatRange (0:false, 1:true)> Optional -c <readFromConfigFileFirst(0:false, 1:true)> Optional -w <waitTime> Optional -d <delayTime> -l <communication range> Optional -t <nodeType(0:source Node, 1:routing Node)>
```

#### Node arguments:
The first 3 arguments are required in any time, the others are optionals. which are set with default values inside the code. 

if -r is set to 1 ( true ) then the range of the sensors will start to shorten by the time, very slightly for a short time but after hours of run this might make the node unable to reach others nodes. it is to simulate the effect of battery draining on the MANETs that would affect the power of transmission. 

if -c is set to 1 ( true) then the nodes will read their location and port numbers from the configuration file, in this case -p argument will not take effect but still required. Note that to run this command safely run the filesManager application after initiating and running all the nodes, otherwise the fileManager will overwrite the configuration file. 

use -l to define costume network range for the node other than the default 100 m. This can be helpful to simulate the effect of having different paths for the Data packets and the ACK packets, if each nodes set with different range.  

-t is to define the node type. -t 0 will define the node as source node, note if you set the node to destination node then the node port number have to be +1 from the client port number.When source node receive ACK packet it will transfer it right to the client. -t 1 ( which is the default value ) to set the node to regular node. -t 2 will set the node to destination node, when a DATA packet reach this node the packet will be sent right to the server. 
-w and -d are currently off so they have no effects. 
if -t is set to 0 then the number of of the server node have to be provided with -s <server Node ID>


### Server : 
Most of the optional arguments that made for node can be used with server too. 
```sh
./server -p <port> -n <nodeNumber> -f <configuration file> Optional -d <dataCount>
```
#### server arguments:
-p server port number 
-d is set by default to 1, if set to n ( for example 10)  then the server will receive N packets ( 10 ) average their values then store that for the server front end website to receive. this is helpful when the transfer rate is way faster than the front end refresh rate which is 200 ms. (putting a very high value is not recommended!)
-n for the node number
and 


### fileManager: 
```sh
./fileManager -f <configuration file> -n <number of nodes>
```

### fileManager arguments:
-n to define that number of node that is running ( including source nodes and server nodes). 

-f the configuration file here should be the exact configuration file name put into the nodes and the server.


## Running steps: 
1) run the server 
```sh
./server -p 10140 -n 5 -f conf -d 3
```
2) Run each nodes
make sure that among the nodes there is a source node and a destination node. ( source node port should be +1 from the client port 
example: 
```sh
./node -p 10141 -n 1 -f conf -c 1 -t 0 -s 5
./node -p 10142 -n 2 -f conf -c 1
./node -p 10143 -n 3 -f conf -c 1 
./node -p 10144 -n 4 -f conf -c 1 -t 0 -s 5
```

3) run the fileManager ( make sure you write the same configuration file name for the nodes and the fileManager and you put the right number of nodes that are running)
```sh
./fileManager -f conf -n 4
```
4) Add your Google map api key to the file index.jsp
replace the value [YOUR_MAP_API_KEY] at line 168 with your actual API key from google map developers. 

```sh
sed 's/YOUR_MAP_API_KEY/ACTUAL_KEY/g' index.jsp
```
4) Open the website index.html in a web browser (Preferable Firefox) for real time monitoring

To change the drop rate for each ethernet or manners you have to user the settings file. reading the file will be dynamic for every node. 
### Configuration files
1. settings
contain 2 parameters Ethernet and Manets, There should be drop rate percentage for each value. default (Ethernet: 0.3, Manets: 0.2).

2. conf (nodes configuration file)
This file can be created individually or automated with using the fileManager. In the file each row represent a node in the formate: Node ID hostname port latitude longitude Link <list of connected nodes>
example: Node 1 MYFFNODE.T-mobile.com 10140 32.603138 -85.492279 Link 2 3 4 5 6 7
## Special features: 
1. implementing propagation delay, fast fading and slow fading effects (drop rate effect range from 0 to 1)
2. if the packet is critical it will be sent on both ethernet and MANETs concurrently to make sure of delivery with less delay if packet dropped mid way. 
3. simulating the effect of battery drain, which affect the range of nodes when running for long time (it is turned off by default)
4. able to set up different range for each node, simulating different wireless modules implemented in different nodes (the default value is 100 meters).
5. Different setup for the nodes location, read from file or manually put in the program argument, if none of these are provided the nodes will run in scatter mode, starting from the the first Responder position and scattering randomly around from that point. ( scattering point is set into Shelby center in Auburn Alabama)
6. using real latitude and longitude instead of x and y for the location, better for simulating on a real google map.  
7. Multiple Source nodes can be employed those will produce sensors reading and also route packets in the Manets network.  

 
