#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <sys/file.h>
#define MAX_SIZE 51200
#define N 10
#define MAX_NODES 50
typedef int bool;
#define true 1
#define false 0


/******************************  Date Structure Declration  **********************************/
// Type ( 'D' : data, 'A': ACK, 'N':NAK, 'P': prope)
struct packet{
    char type[1];
    short source;
    short dist;
    short previous;
    int seq;
    bool isCritical;
    int heartRate;
    float oxygenLevel;
    float latitude;
    float longitude;
    float toxicgas;
};
//***Underwork***

struct oneHop{
    short id;
    char state[1];
    int sequence;
    struct timeval updateTime;
};
struct twoHop{
    short id;
    short access;
};

// Type node to store the node informations for easier access
struct node{
    short id;
    char name[50];
    int port;
    float latitude;
    float longitude;
    int links[MAX_NODES];
};

struct sensorsData{
    int heartRate;
    float oxygenLevel;
    float toxicGas;
    float latitude;
    float longitude;
};
struct networkInfo{
    float range;
    float dropRate;
    float ethernetDropRate;
    float manetsDropRate;
    float ethernetBandidth;  // packets per seconds
    float manetsBandidth;  // packets per seconds
    int socket;
    short serverID;
    char fileName[50];
};

//global nodes variable
struct node connectedNodes[MAX_NODES];
struct oneHop oneHopNeighbors[MAX_NODES];
struct twoHop twoHopNeighbors[MAX_NODES];
short MPRSelectors[MAX_NODES];
short heardFrom[MAX_NODES];
int MPRSelectorsN = 0;
int oneHopN =0;
int twoHopN =0;
int links =0;
int heardFromN = 0;
struct networkInfo networkData;
struct node myInfo;
struct node serverInfo;
int latestSeq[MAX_NODES];
int lastAck = 0;
int lastDataAck[MAX_NODES];
//Initial sensors data
struct sensorsData sensors;
/********************************  Functions decleration  ***********************************/
void logData(const char *fileName, struct packet packetInfo, const char *status, const char *network);
void *heartRateThread(void *result);
void *oxygenLevelThread(void *result);
void *toxicGasThread(void *result);
void *locationThread(void *result);
void *depleatRangelThread(void *result);
bool gremlin(float rate);
bool isCritical(int heartRate, float oxygenLevel);
float getLongitude(float base);
float getLatitude(float base);
float getOxygen( float base);
float getToxicGasLevel( float base);
int getHeartRate( int base);
bool isSensorDataChanged(struct sensorsData previousReadings, struct sensorsData newReadings);
void writeBaseInfoToFile(const char *fileName, struct node myInfo);
void findConnectedNodes(const char *fileName,struct node myInfo, float radius);
int getLocationFromFile(const char *fileName,int nodeID);
bool isNodeInRadious(struct node myInfo, struct node nextNode, float radius);
void delay(float msDelay);
float getDistance(struct node myInfo, struct node nextNode);
struct node findTargetNode(const char *fileName,short targetID);
bool isSensorLocationChanged(struct node myInfo, struct sensorsData newReadings);
void clearAllLinks();
void sendData(int seqNo, struct node serverInfo, struct node myInfo);
float calculateDropRate(struct node myNode, struct node sourceNode, float maxRange);
float slowFadding(struct node myNode, struct node sourceNode, float maxRange);
float fastFadding(struct node myNode, struct node sourceNode, float maxRange);
float propegationLoss(struct node myNode, struct node sourceNode, float maxRange);
bool sendPacket(int sock,struct packet buffPacket, struct node distination, float dropRate);
bool sendEthernet(int sock, struct packet buffPacket, float dropRate);
bool sendMANETs(int sock, struct packet buffPacket, float dropRate);
bool waitForACK(struct packet buffPacket);
float pingEtherNet(struct packet pingPacket);
float pingMANETs(struct packet pingPacket);
float pingBothConnection(struct packet pingPacket);
int arrayToFloat(short arr[], int base);
int floatToArray(short arr[], int numb, int base);
void getSettingsFromFile(const char *fileName);
bool isNodeInOneHopTable(short nodeID);
bool isNodesInTwoHopTable(short nodeID, short accessFrom);
void generateState(struct oneHop *oneHopTable,struct twoHop *twoHopTable);
void updateTwoHopTable(int listDigits,short source, struct node myInfo, int base);
struct packet setupHelloMessage();
void updateHeardFromTable(struct packet buffPacket);
void updateOneHopTable(struct packet buffPacket, struct node myInfo);
struct node findNodeFromConnectedList(short nodeID);
int findMPRList(short selectedList[], int max, int min);
int selectListHeardFrom(short selectedList[], int max, int min);
int selectList(short selectedList[], int max, int min);
bool didThisNodeSelectedMeAsMPR(short list[],struct node myInfo);
bool isNodeBiDirectional(short list[], struct node myInfo);
bool isNodeMPRSelector(short NodeID);
bool updateTables(struct packet buffPacket, struct node myInfo);
void *sendHelloMessagesThread();
void *sendDataThread();
bool isTargetInOneNeighborsList(short target);
void routPacketOLSR(struct packet buffer);
void deleteLinksbySource(short sourceID);
void deleteHeardFromByID(short sourceID);
void deleteOutDatedNeighbbors(double timout);
void *deleteOutDatedNeighbborsThread();
/*********************************** Main function ***************************************/
int main(int argc, char *argv[]){
    //Basic Variables
    unsigned int addr_len;
    int sock;
    struct sockaddr_in server_addr, client_addr, my_addr;
    int portNumber;
    int r =0;
    int waitTime = 40000; //set timeout for 40 ms
    double delayTime = 0.0;
    short nodeNumber;
    networkData.range = 100;
    bool depleatRange = false;
    int type ;
    type = 1;
    char configFile[50];
    bool readConfigFileFirst = false;
    
    // packets variables, one for regular packets and one for ACK packets
    struct packet buffPacket;
    bzero(&(myInfo.links),sizeof(myInfo.links));
    bzero(&(lastDataAck),sizeof(lastDataAck));
    bzero(&(latestSeq),sizeof(latestSeq));
    links = 0;
    
    sensors.latitude = getLatitude(32.60618);
    sensors.longitude = getLatitude(-85.48703);
    sensors.heartRate = getHeartRate(70);
    sensors.oxygenLevel = getOxygen(100);
    sensors.toxicGas = getToxicGasLevel(0.5);
    //    // Getting the program arguments and managing errors in this part
    if (argc <7 ){
        fprintf(stderr,"ERROR, Usage:%s -p <myPort> -n <nodeNumber> -f <configuration file>Optional -r <depleatRange(0:false, 1:true)> Optional -c <readFromConfigFileFirst(0:false, 1:true)>Optinal -w <waitTime> Optional -d <delayTime> -l <comunication range> Optional -t <nodeType(0:source, 1:node, 2:distination)>\n",argv[0]);
        exit(1);
    }
    int in;
    for(in=1;in<6;in+=2){
        if (strncmp(argv[in],"-p",2)==0){
            portNumber = atoi(argv[in+1]);
        }
        else if (strncmp(argv[in],"-n",2)==0){
            nodeNumber =  atoi(argv[in+1]);
        }
        else if (strncmp(argv[in],"-f",2)==0){
            strcpy(configFile, argv[in+1]);
        }
        else {
            fprintf(stderr,"ERROR, Wrong argument type '%s'\n",argv[in]);
            fprintf(stderr,"ERROR, Usage:%s -p <myPort> -n <nodeNumber> -f <configuration file>Optional -r <depleatRange(0:false, 1:true)> Optional -c <readFromConfigFileFirst(0:false, 1:true)>Optinal -w <waitTime> Optional -d <delayTime> -l <comunication range> Optional -t <nodeType(0:source, 1:node, 2:distination)>\n",argv[0]);
            exit(1);
        }
    }
    // in case of having optional program arguments
    if(argc > 7){
        for(in=7;in<argc;in+=2){
            if (strncmp(argv[in],"-w",2)==0){
                waitTime= atof(argv[in+1])* 1000000;
            }
            else if (strncmp(argv[in],"-r",2)==0){
                depleatRange = atoi(argv[in+1]);
            }
            else if (strncmp(argv[in],"-s",2)==0){
                networkData.serverID = atoi(argv[in+1]);
            }
            else if (strncmp(argv[in],"-c",2)==0){
                readConfigFileFirst = atoi(argv[in+1]);
            }
            else if (strncmp(argv[in],"-d",2)==0){
                delayTime = atoi(argv[in+1]);
            }
            else if (strncmp(argv[in],"-la",3)==0){
                sensors.latitude = atof(argv[in+1]);
            }
            else if (strncmp(argv[in],"-lo",3)==0){
                sensors.longitude = atof(argv[in+1]);
            }
            else if ( strncmp(argv[in],"-l",2)==0 ){
                networkData.range = atof(argv[in+1]);
            }
            else if ( strncmp(argv[in],"-t",2)==0 ){
                type= atoi(argv[in+1]);
            }
            else{
                fprintf(stderr,"ERROR, Unknown optional argument type '%s'\n",argv[in]);
                fprintf(stderr,"ERROR, Usage:%s -p <myPort> -n <nodeNumber> -f <configuration file> Optional -r <depleatRange(0:false, 1:true)> Optional -c <readFromConfigFileFirst(0:false, 1:true)>Optinal -w <waitTime> Optional -d <delayTime> -l <comunication range> Optional -t <nodeType(0:source, 1:node, 2:distination)>\n",argv[0]);
                exit(1);
            }
        }
    }
    if(type == 0 && networkData.serverID ==0){
        fprintf(stderr,"ERROR, Source node must have a server ID provided using argument -s <serverNodeID> \n");
        exit(1);
    }
    if(readConfigFileFirst){
        portNumber = getLocationFromFile(configFile,nodeNumber);
        printf("--file info read successfully\n");
    }
    strcpy(networkData.fileName, configFile);
    //Data File setup
    char baseFileName[10];
    char fileName[30];
    sprintf(baseFileName, "node");
    sprintf(fileName, "%s%d.txt",baseFileName ,nodeNumber);
    // figuring out my the computer name on the network
    char hostName[50];
    hostName[49] = '\0';
    gethostname(hostName, 49);
    struct hostent* myName;
    myName = gethostbyname(hostName);
    //setting up the node information
    myInfo.id = nodeNumber;
    myInfo.port = portNumber;
    strcpy(myInfo.name, hostName);
    //myInfo.name = hostName;
    myInfo.latitude = sensors.latitude;
    myInfo.longitude = sensors.longitude;
    writeBaseInfoToFile(fileName, myInfo);
    getSettingsFromFile("settings");
    //setting up a pthread for updating the location every 0.5 seconds
    pthread_t locationThreadID, helloThreadID, deleteOutDatedNeighbborsThreadID;
    pthread_t heartRateThreadID, oxygenLevelThreadID, toxicGasThreadID, dataThreadID;
    pthread_t RangeThreadID;
    int err;
    err = pthread_create(&locationThreadID, NULL, &locationThread, &sensors);
    if (err>0){
        perror("LocationThread: ");
    }
    err =pthread_create(&helloThreadID, NULL, &sendHelloMessagesThread, NULL);
    if (err>0){
        perror("HelloMessages: ");
    }
    err= pthread_detach(locationThreadID) ;
    if (err>0){
        perror("HelloMessages Detach: ");
    }
    pthread_create(&deleteOutDatedNeighbborsThreadID, NULL, &deleteOutDatedNeighbborsThread, NULL);
    
    if(depleatRange){
        pthread_create(&RangeThreadID, NULL, &depleatRangelThread, &networkData);
    }
    // if the Node is source node generate the other sensor data
    if (type == 0){
        serverInfo = findTargetNode(networkData.fileName,networkData.serverID);
        pthread_create(&heartRateThreadID, NULL, &heartRateThread, &sensors);
        pthread_create(&oxygenLevelThreadID, NULL, &oxygenLevelThread, &sensors);
        pthread_create(&toxicGasThreadID, NULL, &toxicGasThread, &sensors);
        err = pthread_create(&dataThreadID, NULL, &sendDataThread, NULL);
        if (err>0){
            perror("Send Data Thread: ");
        }
        err= pthread_detach(dataThreadID) ;
        if (err>0){
            perror("Send Data Thread Detach: ");
        }
    }
    
    // defining the time out for reciving the ACK
    struct timeval timeout = {0,waitTime};
    
    printf("---------------------------------------------------------------------\n");
    printf("              Node  : %s  type %d         \n", hostName, type);
    printf("---------------------------------------------------------------------\n");
    bzero(&(myInfo.links),MAX_NODES);
    
    // Defining the UDP packet ( SOCK_DGRAM )
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }
    //setting up the address informaton
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNumber);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero),8);
    
    //seting up my address
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portNumber);
    my_addr.sin_addr = *((struct in_addr *)myName->h_addr);
    bzero(&(my_addr.sin_zero),8);
    
    // Binding the socket with the server address
    if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        perror("Bind");
        exit(1);
    }
    networkData.socket = sock;
    addr_len = sizeof(struct sockaddr);
    printf("-- Waiting for requests --\n");
    //setting up a timeout for the recive command
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
    int i;
    while(1){
        buffPacket.seq = 0;
        if(isSensorLocationChanged(myInfo, sensors)){
            myInfo.latitude = sensors.latitude;
            myInfo.longitude = sensors.longitude;
            bzero(&(myInfo.links),sizeof(myInfo.links));
            links =0;
            findConnectedNodes(configFile,myInfo, networkData.range);
            //printf("\n **my new location: <%f, %f> ", myInfo.latitude, myInfo.longitude);
            //printf("Nodes in Range:");
            for (i = 0; i<links;i++){
                myInfo.links[i] = connectedNodes[i].id;
                //    printf(" %d",myInfo.links[i]);
            }
            //   printf("\n");
            //   printf("range: %f\n", networkData.range);
            writeBaseInfoToFile(fileName, myInfo);
            getSettingsFromFile("settings");
        }
        //Determin the type of message recived :
        // 1: D, route packet using OLSR or Flooding
        // 2: A, route packet using OLSR or Flooding OR recive if source node
        // 3: H, update routing table
        //waiting for packets to be relayed
        r = recvfrom(sock,&buffPacket,MAX_SIZE, 0,(struct sockaddr *)&client_addr, &addr_len);
        if (r>0){
            if (buffPacket.type[0] =='H'){
                printf("*** Hello message recived from %hd ***\n", buffPacket.source);
                updateTables(buffPacket, myInfo);
                printf("close By Nodes:");
                for(i=0; i<links; i++){
                    printf(" %hd", connectedNodes[i].id);
                }
                printf("\n");
                printf("Heard from Nodes:");
                for(i=0; i<heardFromN; i++){
                    printf(" %hd", heardFrom[i]);
                }
                printf("\n");
                printf("OneHop Neighbors:");
                for(i=0; i<oneHopN; i++){
                    printf(" %hd(%c)", oneHopNeighbors[i].id, oneHopNeighbors[i].state[0]);
                }
                printf("\nTwoHop Neighbors:");
                for(i=0; i<twoHopN; i++){
                    printf(" %hd from %hd. ", twoHopNeighbors[i].id, twoHopNeighbors[i].access);
                }
                printf("\n \n");
                
                //                printf("My MPR selectors:");
                //                for(i=0; i<MPRSelectorsN; i++){
                //                    printf(" %hd", MPRSelectors[i]);
                //                }
                //                printf("\n");
            }
            if(type == 1 && buffPacket.type[0] == 'A' && (lastDataAck[buffPacket.source] < buffPacket.seq || lastDataAck[buffPacket.source] > buffPacket.seq + 10)){
                
                printf("* ACK Packet %d recived for routing \n",buffPacket.seq );
                lastDataAck[buffPacket.source] = buffPacket.seq;
                routPacketOLSR(buffPacket);
            }
            if(type == 0 && buffPacket.type[0] == 'A' && (lastAck < buffPacket.seq || lastAck > buffPacket.seq + 10)){
                if(buffPacket.dist == myInfo.id){
                    lastAck = buffPacket.seq;
                }
                else{
                    printf("* ACK Packet %d recived for routing \n",buffPacket.seq );
                    lastDataAck[buffPacket.source] = buffPacket.seq;
                    routPacketOLSR(buffPacket);
                }
            }
            if(buffPacket.type[0] =='D' && (latestSeq[buffPacket.source] < buffPacket.seq || latestSeq[buffPacket.source] > buffPacket.seq + 10 ) && buffPacket.source != myInfo.id){
                latestSeq[buffPacket.source] = buffPacket.seq;
                printf("* Data Packet %d recived for routing \n",buffPacket.seq );
                routPacketOLSR(buffPacket);
            }
            r = 0;
        }
        
    }
    return 0;
}

/***************************  Functions Defnitions **********************************/

/* gremlin function
 takes parameter of rate
 return true if the random number genetrated is less or equal the rate
 otherwise return false
 when used to determine sending a packet status,  true means the packet will be dropped
 */
bool gremlin(float rate)
{
    int tempRate=rate*100;
    int rateMatch=rand()%100;
    if(rateMatch<tempRate){
        return true;
    }else{
        return false;
    }
}

//delay function takes input in milisecond
void delay(float msDelay){
    struct timeval t1, t2;
    double elapsedTime = 0;
    
    // start timer
    gettimeofday(&t1, NULL);
    while(elapsedTime < msDelay){
        gettimeofday(&t2, NULL);
        
        // compute the elapsed time in millisec
        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    }
}
// function to convert aray of digits into an integer, input arr[10]={3,4,2,5,6,3,9}; number of node is limited to the size of int
int arrayToFloat(short arr[], int base){
    int index=0;
    int numberFlt=0;
    while(arr[index] > 0){
        numberFlt*=base;
        numberFlt+=arr[index];
        index++;
    }
    
    return numberFlt;
}
//function to extract array information from an integer, input numbf=34567
int floatToArray(short arr[], int numb, int base){
    int size = 0;
    while(numb%base > 0){
        arr[size]=numb%base;
        numb/=base;
        size++;
    }
    return size;
}

// Updating the longitude randomely
float getLongitude(float base){
    
    float changeLongitude;
    if(gremlin(0.65) == true){
        changeLongitude=base*1000000.0+ (float)(rand()%10);
    }else{
        changeLongitude=base*1000000.0- (float)(rand()%10);
    }
    return changeLongitude/1000000.0;
}

// Updating the latitude randomely
float getLatitude(float base){
    float changeLatitude;
    if(gremlin(0.65) ==true){
        changeLatitude=base*1000000.0+ (float)(rand()%10);
    }
    else{
        changeLatitude=base*1000000.0- (float)(rand()%10);
    }
    return changeLatitude/1000000.0;
}

// writing the base node information to file
void writeBaseInfoToFile(const char *fileName, struct node myInfo){
    FILE *fp;
    fp = fopen(fileName, "w+");
    if(fp==NULL){
        printf("Error opening file\n");
        return ;
    }
    int lockresult = flock(fileno(fp), LOCK_EX);
    char lineBuffer[100];
    char fileBuffer[200];
    sprintf(lineBuffer, "Node %hd %s %d %f %f Link",myInfo.id, myInfo.name,myInfo.port, myInfo.latitude, myInfo.longitude );
    strcpy(fileBuffer,lineBuffer);
    if(links > 0){
        int link =0;
        while (myInfo.links[link] >0 && link < links){
            sprintf(lineBuffer, " %d", myInfo.links[link]);
            strcat(fileBuffer,lineBuffer);
            link ++;
        }
    }
    strcat(fileBuffer,"\n");
    fputs(fileBuffer,fp);
    fclose(fp);
    int release = flock(fileno(fp), LOCK_UN);
}

//Finding the distance between 2 nodes in meters using the Equirectangular approximation method
// source used for the distance method http://www.movable-type.co.uk/scripts/latlong.html
//source used for valitdiating the correction of the function http://andrew.hedges.name/experiments/haversine/
float getDistance(struct node myInfo, struct node nextNode){
    float R  = 6373e3; //aproximation for earth radius in m
    float myLatitudeRadian = (myInfo.latitude * M_PI) /180;
    float nextLatitudeRadian =(nextNode.latitude * M_PI) /180;
    float differenceLongitude = (fabsf(myInfo.longitude - nextNode.longitude) * M_PI /180);
    float x = differenceLongitude * cos((myLatitudeRadian +nextLatitudeRadian)/2);
    float y = fabsf(myLatitudeRadian-nextLatitudeRadian);
    float distance = (sqrt(x*x + y*y) * R);
    return distance;
}

// determine if 2 nodes are in the radius (in meters) from each others
bool isNodeInRadius(struct node myInfo, struct node nextNode, float radius){
    float distance = getDistance(myInfo, nextNode);
    if (distance<=radius)
        return true;
    else
        return false;
}

//simulate slow fading effect based on the distance between 2 nodes. Return the drop rate.
float slowFadding(struct node myNode, struct node sourceNode, float maxRange){
    float dropRate;
    float distance = getDistance(myNode, sourceNode);
    // need to map the distance from range to 180 degree
    if(distance !=0){
        distance = distance *(180/maxRange);
    }
    float rDistance = ( distance* M_PI) /180;
    dropRate = fabs(sin(rDistance));
    return networkData.manetsDropRate * dropRate;
}
//simulate fast fading effect based on the distance between 2 nodes. Return the drop rate.
float fastFadding(struct node myNode, struct node sourceNode, float maxRange){
    float dropRate;
    float distance = getDistance(myNode, sourceNode);
    // need to map the distance from range to 180 degree
    if(distance !=0){
        distance = distance *(180/maxRange);
    }
    float rDistance = ( distance* M_PI) /180;
    dropRate = fabs(sin((maxRange / 20) * rDistance));
    return networkData.manetsDropRate * dropRate;
}
//simulate propegation loss effect based on the distance between 2 nodes. Return the drop rate.
float propegationLoss(struct node myNode, struct node sourceNode, float maxRange){
    float dropRate;
    float distance = getDistance(myNode, sourceNode);
    dropRate = pow((distance/maxRange),2);
    return networkData.manetsDropRate * dropRate;
}
//calculate the Drop rate
float calculateDropRate(struct node myNode, struct node sourceNode, float maxRange){
    float propegationDropRate = propegationLoss(myNode, sourceNode, maxRange);
    if(gremlin(0.14)){
        float slowFaddingDropRate = slowFadding(myNode, sourceNode, maxRange);
        if(slowFaddingDropRate > propegationDropRate){
            printf("Slow Fading Effect is affecting the connection! %f \n", slowFaddingDropRate);
            return slowFaddingDropRate;
        }
        else{
            if ( propegationDropRate > 0.15 )
                printf("propegation Loss Effect is affecting the connection, drop rate is high! %f\n", propegationDropRate);
            return propegationDropRate;
        }
    }
    else if (gremlin(0.15)){
        float fastFaddingDropRate = fastFadding(myNode, sourceNode, maxRange);
        printf("Fast Fading Effect is affecting the connection! %f\n", fastFaddingDropRate);
        return fastFaddingDropRate;
    }
    else{
        if ( propegationDropRate > 0.15 )
            printf("propegation Loss Effect is affecting the connection, drop rate is high! %f\n", propegationDropRate);
        return propegationDropRate;
    }
}
// scan the file to find specific node information.. not implements because we chose to update the node from its own GPS instead or reading its info from the file.
int getLocationFromFile(const char *fileName,int nodeID){
    struct node targetNode, readNode;
    char *lineBuffer= NULL;
    size_t length = 200;
    ssize_t bytesRead;
    FILE *fp;
    fp = fopen(fileName, "r");
    if(fp>0){
        //printf("file Opened \n");
        while ((bytesRead =getline(&lineBuffer, &length, fp))> 0){
            sscanf(lineBuffer, "Node %hd %s %d %f %f", &readNode.id, readNode.name, &readNode.port, &readNode.latitude, &readNode.longitude);
            //printf("found node: %d \n", readNode.id);
            if (readNode.id == nodeID){
                targetNode =readNode;
                break;
            }
        }
        fclose(fp);
        sensors.latitude = targetNode.latitude;
        sensors.longitude = targetNode.longitude;
        return targetNode.port;
    }
    else{
        printf("Error opening the Configuration File %ss\n", fileName);
        return 0;
    }
}

// Reading the configuration file and finding the nodes in range based on the location and using the isNodeInRadius function
void findConnectedNodes(const char *fileName,struct node myInfo, float radius){
    struct node readNode;
    char *lineBuffer= NULL;
    size_t length = 200;
    ssize_t bytesRead;
    FILE *fp;
    links = 0;
    bzero(&(connectedNodes), sizeof(connectedNodes));
    if ((fp = fopen(fileName, "r"))>0){
        int lockresult = flock(fileno(fp), LOCK_EX);
        if(lockresult ==0){
            int count = 0;
            int size = 0;
            //links = 0;
            //clearAllLinks();
            while ((bytesRead =getline(&lineBuffer, &length, fp))> 0){
                size = sscanf(lineBuffer, "Node %hd %s %d %f %f", &readNode.id, readNode.name, &readNode.port, &readNode.latitude, &readNode.longitude);
                if(size >1){
                    if (isNodeInRadius(myInfo, readNode, radius) == true && myInfo.id != readNode.id){
                        connectedNodes[count] = readNode;
                        count ++;
                        links = count;
                    }
                }
            }
            //free(lineBuffer);
            int release = flock(fileno(fp), LOCK_UN);
        }
        fclose(fp);
    }
    else {
        printf("Error opening the file\n");
        return ;
    }
}

// Reading the configuration file and finding the nodes in range based on the location and using the isNodeInRadius function
struct node findTargetNode(const char *fileName,short targetID){
    struct node readNode;
    char *lineBuffer= NULL;
    size_t length = 200;
    ssize_t bytesRead;
    FILE *fp;
    if ((fp = fopen(fileName, "r"))>0){
        int lockresult = flock(fileno(fp), LOCK_EX);
        if(lockresult ==0){
            int size = 0;
            //links = 0;
            //clearAllLinks();
            while ((bytesRead =getline(&lineBuffer, &length, fp))> 0){
                size = sscanf(lineBuffer, "Node %hd %s %d %f %f", &readNode.id, readNode.name, &readNode.port, &readNode.latitude, &readNode.longitude);
                if(size >1 && readNode.id == targetID){
                    break;
                }
            }
            //free(lineBuffer);
            int release = flock(fileno(fp), LOCK_UN);
        }
        fclose(fp);
    }
    else {
        printf("Error opening the file\n");
        readNode.id =-1;
        return readNode;
    }
    return readNode;
}
void *locationThread(void *result){
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 500 * 1000000;
    //original location set 50 metter range if reached rest to original location
    struct sensorsData previous = sensors;
    while(1){
        nanosleep(&time, (struct timespec *)NULL);
        //delay(500);
        struct sensorsData *sensors = (struct sensorsData *)result;
        sensors->latitude = getLatitude(sensors->latitude);
        sensors->longitude = getLongitude(sensors->longitude);
    }
    //pthread_exit(NULL);
    return NULL;
}

// function to determine if node location is updated or not.
bool isSensorLocationChanged(struct node myInfo, struct sensorsData newReadings){
    if (myInfo.latitude != newReadings.latitude)
        return true;
    if (myInfo.longitude != newReadings.longitude)
        return true;
    return false;
}
void clearAllLinks(){
    int nodeNo, linkNo;
    for (nodeNo=0; nodeNo<MAX_NODES;nodeNo++){
        for (linkNo=0; linkNo<MAX_NODES;linkNo++){
            connectedNodes[nodeNo].links[linkNo] = -1;
        }
    }
}

// Oxygen level simulation function
float depleatRange(float base){
    float range;
    if(gremlin(0.55) ==true ){
        range=base * 100 - (rand()%5);
        range = range /100;
    }
    else{
        range = base;
    }
    //limiting minimum to 0% t
    if (range <0)
        range = 00.0;
    return range ;
}
// main air pack sensor thread update every 2 seconds
void *depleatRangelThread(void *result){
    struct timespec time;
    time.tv_sec = 2;
    time.tv_nsec = 0;
    while(1){
        nanosleep(&time, (struct timespec *)NULL);
        //delay(2000);
        struct networkInfo  *networkRange = (struct networkInfo *)result;
        networkRange->range = depleatRange(networkRange->range);
    }
    //pthread_exit(NULL);
    return NULL;
}
//send Function
bool sendPacket(int sock, struct packet buffPacket, struct node distination, float dropRate){
    //setting up the server
    struct hostent* serverName;
    serverName = gethostbyname(distination.name);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(distination.port);
    server_addr.sin_addr= *((struct in_addr *)serverName->h_addr);
    bzero(&(server_addr.sin_zero),8);
    int buffSize = sizeof(buffPacket);
    if(gremlin(dropRate)){
        printf("packet %d dropped from  node %hd!!\n",buffPacket.seq, distination.id);
        return false;
    }
    else {
        int n = sendto(sock, &buffPacket,buffSize, 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
        printf("packet %d sent to node %hd size:%d\n",buffPacket.seq, distination.id, buffSize);
        return true;
    }
}

// function to determine if new readings are recorded from any sensor
bool isSensorDataChanged(struct sensorsData previousReadings, struct sensorsData newReadings){
    if(previousReadings.heartRate != newReadings.heartRate)
        return true;
    if (previousReadings.oxygenLevel != newReadings.oxygenLevel)
        return true;
    if (previousReadings.latitude != newReadings.latitude)
        return true;
    if (previousReadings.longitude != newReadings.longitude)
        return true;
    if (previousReadings.toxicGas != newReadings.toxicGas)
        return true;
    
    return false;
}

// Oxygen level simulation function
float getOxygen( float base){
    float oxygenChange;
    if(gremlin(0.55) ==true ){
        oxygenChange=base * 100 - (rand()%5);
        oxygenChange = oxygenChange /100;
    }
    else{
        oxygenChange = base;
    }
    //limiting the range from 0% to 100%
    if (oxygenChange <0)
        oxygenChange = 00.0;
    return oxygenChange ;
}

float getToxicGasLevel( float base){
    float toxicGas;
    if(gremlin(0.5) ==true ){
        toxicGas=base * 100+ rand()%10;
    }
    else{
        toxicGas=base * 100 - rand()%10;
    }
    //limiting the range from 0% to 100%
    toxicGas = toxicGas /100;
    if (toxicGas >100)
        toxicGas = 100.0;
    if (toxicGas <0)
        toxicGas = 00.0;
    return toxicGas ;
}
//Heart Rate simulation function
int getHeartRate( int base){
    int heartRateChange;
    if(gremlin(0.55) == true){
        heartRateChange=base + rand()%5;
    }
    else{
        heartRateChange=base - rand()%5;
    }
    //limiting the range from 0 to 220
    if (heartRateChange > 220)
        heartRateChange = 220;
    if (heartRateChange < 0)
        heartRateChange = 0;
    return heartRateChange;
}

// main heart rate sensor thread update every 1 second
void *heartRateThread(void *result){
    struct timespec time;
    time.tv_sec = 1;
    time.tv_nsec = 0;
    while(1){
        nanosleep(&time, (struct timespec *)NULL);
        //delay(1000);
        struct sensorsData *sensors = (struct sensorsData *)result;
        sensors->heartRate = getHeartRate(sensors->heartRate);
    }
    //pthread_exit(NULL);
    return NULL;
}
// main air pack sensor thread update every 2 seconds
void *oxygenLevelThread(void *result){
    struct timespec time;
    time.tv_sec = 2;
    time.tv_nsec = 0;
    while(1){
        nanosleep(&time, (struct timespec *)NULL);
        //delay(2000);
        struct sensorsData *sensors = (struct sensorsData *)result;
        sensors->oxygenLevel = getOxygen(sensors->oxygenLevel);
    }
    //pthread_exit(NULL);
    return NULL;
}
// main toxic gas sensor thread update every 0.25 second
void *toxicGasThread(void *result){
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 250 * 1000000;
    while(1){
        nanosleep(&time, (struct timespec *)NULL);
        // delay(250);
        struct sensorsData *sensors = (struct sensorsData *)result;
        sensors->toxicGas = getToxicGasLevel(sensors->toxicGas);
    }
    //pthread_exit(NULL);
    return NULL;
}

//function to determn if the reading data from the sensors are life threatening or not.
bool isCritical(int heartRate, float oxygenLevel){
    bool status = false;
    if ( heartRate > 150 || heartRate <55)
        status = true;
    if (oxygenLevel < 85)
        status = true;
    return status;
}
//return node from the connected long list which were optained from the configuration file
struct node findNodeFromConnectedList(short nodeID){
    struct node targetNode;
    targetNode.id = 0;
    int i;
    for(i=0;i<links;i++){
        if(nodeID == connectedNodes[i].id){
            targetNode = connectedNodes[i];
            return targetNode;
        }
    }
    return targetNode;
}
//**Send the message using ethernet connection **
bool sendEthernet(int sock, struct packet buffPacket, float dropRate){
    struct packet buffPacketR;
    struct tm *tm;
    buffPacketR=buffPacket;
    int seqNo = buffPacketR.seq;
    struct timeval t0;
    //struct node serverInfo;
    //serverInfo = findTargetNode(networkData.fileName, buffPacket.dist);
    struct hostent* serverName;
    serverName = gethostbyname(serverInfo.name);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverInfo.port);
    server_addr.sin_addr= *((struct in_addr *)serverName->h_addr);
    bzero(&(server_addr.sin_zero),8);
    
    int buffSize = sizeof(struct packet);
    if (gremlin(dropRate)){
        gettimeofday(&t0,NULL);
        tm = localtime(&t0.tv_sec);
        printf("Packet %d Dropped at %d:%02d:%02d:%d !!!\n",seqNo, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000);
        return false;
    }
    else{
        int n = sendto(sock, &buffPacketR,buffSize, 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
        gettimeofday(&t0,NULL);
        tm = localtime(&t0.tv_sec);
        printf("Packet %d sent at %d:%02d:%02d:%d size: %d Bytes using Ethernet\n",seqNo, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000, buffSize);
        return true;
        
    }
}



//**Send the message using MANETs ad hoc connection **
bool sendMANETs(int sock, struct packet buffPacket, float dropRate){
    struct packet buffPacketR;
    buffPacketR=buffPacket;
    struct hostent* serverName;
    struct sockaddr_in server_addr;
    struct node nodeInfo;
    bool status = false;
    int MPRCount = 0;
    int seqNo = buffPacketR.seq;
    struct tm *tm;
    struct timeval t0;
    int buffSize = sizeof(struct packet);// 4 + 3 * sizeof(int) + 3 * sizeof(float) + 3 * sizeof(struct sockaddr_in)+12 ;
    if(oneHopN>0){
        int i;
        for(i=0;i<oneHopN;i++){
            if(oneHopNeighbors[i].state[0] =='M' && oneHopNeighbors[i].id>0){
                nodeInfo = findNodeFromConnectedList(oneHopNeighbors[i].id);
                if(nodeInfo.id>0){
                    dropRate = calculateDropRate(myInfo, nodeInfo, networkData.range);
                    serverName = gethostbyname(nodeInfo.name);
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(nodeInfo.port);
                    server_addr.sin_addr= *((struct in_addr *)serverName->h_addr);
                    bzero(&(server_addr.sin_zero),8);
                    MPRCount++;
                    if (gremlin(dropRate)){
                        gettimeofday(&t0,NULL);
                        tm = localtime(&t0.tv_sec);
                        printf("Packet %d Dropped at %d:%02d:%02d:%d !!!\n",seqNo, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000);
                        //  return false;
                    }
                    else{
                        int n = sendto(sock, &buffPacketR,buffSize, 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
                        gettimeofday(&t0,NULL);
                        tm = localtime(&t0.tv_sec);
                        printf("Packet %d sent at %d:%02d:%02d:%d size: %d Bytes - using MANETs Node %hd\n",seqNo, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000, buffSize, nodeInfo.id);
                        status = true;
                    }
                }
                else{
                    printf("* couldn't find node %hd in the connection list\n", oneHopNeighbors[i].id );
                }
            }
        }
        //}
    }
    else {
        printf("X No neighbor nodes connected !!! mannets network is unavailable !\n");
    }
    if(MPRCount == 0){
        generateState(oneHopNeighbors, twoHopNeighbors);
        printf("X There were no MPR nodes !! forwarding packets faild !\n");
    }
    if(status == true)
        return true;
    else
        return false;
}
//**wait for packet ACK that have been previously sent **
bool waitForACK(struct packet buffPacket){
    struct tm *tm;
    struct timeval t0;
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 40000000; //wait 40 ms to insure that ACK packet recived if it has been sent from the server
    nanosleep(&time, (struct timespec *)NULL);
    //printf("Sequence: %d\n", buffPacket.seq);
    //printf("Last Known ACK: %d \n", lastAck);
    if(lastAck == buffPacket.seq){
        gettimeofday(&t0,NULL);
        tm = localtime(&t0.tv_sec);
        printf("ACK for packet %d at %d:%02d:%02d:%d \n",buffPacket.seq, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000);
        return true;
    }
    else{
        gettimeofday(&t0,NULL);
        tm = localtime(&t0.tv_sec);
        printf("NACK for packet %d at %d:%02d:%02d:%d !\n",buffPacket.seq, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000);
        return false;
    }
}
//function to log data into log file.
void logData(const char *fileName, struct packet packetInfo, const char *status, const char *network){
    struct tm *tm;
    FILE *fp;
    struct timeval t0;
    gettimeofday(&t0,NULL);
    tm = localtime(&t0.tv_sec);
    char lineBuffer[200];
    sprintf(lineBuffer,"%d:%02d:%02d:%d, %d, %d, %3d, %.2f, %.3f, %.6f, %.6f, %s, %s\n",tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000, packetInfo.seq, packetInfo.isCritical, packetInfo.heartRate, packetInfo.oxygenLevel, packetInfo.toxicgas, packetInfo.latitude, packetInfo.longitude, status, network);
    fp = fopen(fileName, "a+");
    fprintf(fp, "%s", lineBuffer);
    fclose(fp);
}

float pingEtherNet(struct packet pingPacket){
    struct timeval start,end;
    int counter = 0;
    float packetsPerSecond = 0;
    double elapsedTime = 0;
    bool recived = false;
    gettimeofday(&start, NULL);
    while (!recived && counter <3){
        if(sendEthernet(networkData.socket,pingPacket, networkData.ethernetDropRate)){
            if(waitForACK(pingPacket)){
                recived = true;
                counter++;
                gettimeofday(&end, NULL);
                // compute the elapsed time in millisec
                elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;      // sec to ms
                elapsedTime += (end.tv_usec - start.tv_usec) / 1000.0;   // us to ms
                packetsPerSecond = 1000/elapsedTime;
                logData("log.txt", pingPacket, "ACK", "Ethernet");
            }
            else
                counter++;
        }
        else{
            counter ++;
        }
    }
    if(!recived){
        logData("log.txt", pingPacket, "NACK", "Ethernet");
        printf(" Ethernet connection is down!\n");
    }
    return packetsPerSecond;
}

//function to send packet on manets wait for ACK and compute the packets per seconds connection bandwidth.
float pingMANETs(struct packet pingPacket){
    struct timeval start,end;
    int counter = 0;
    float packetsPerSecond = 0;
    double elapsedTime = 0;
    bool recived = false;
    gettimeofday(&start, NULL);
    while (!recived && counter <3){
        if(sendMANETs(networkData.socket, pingPacket, networkData.manetsDropRate)){
            if(waitForACK(pingPacket)){
                recived = true;
                gettimeofday(&end, NULL);
                // compute the elapsed time in millisec
                elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;      // sec to ms
                elapsedTime += (end.tv_usec - start.tv_usec) / 1000.0;   // us to ms
                packetsPerSecond = 1000/elapsedTime;
                logData("log.txt", pingPacket, "ACK", "MANETs");
            }
            else
                counter++;
        }
        else{
            counter ++;
        }
    }
    if(!recived){
        logData("log.txt", pingPacket, "NACK", "MANETs");
        printf(" MANETs connection is down!\n");
    }
    return packetsPerSecond;
}
//function to send packet on manets wait for ACK and compute the packets per seconds connection bandwidth.
float pingBothConnection(struct packet pingPacket){
    struct timeval start,end;
    int counter = 0;
    float packetsPerSecond = 0;
    double elapsedTime = 0;
    bool recived = false;
    bool sentonMan = false;
    bool sentonEth = false;
    
    gettimeofday(&start, NULL);
    
    while (!recived && counter <6){
        sentonMan = sendMANETs(networkData.socket, pingPacket, networkData.manetsDropRate);
        sentonEth = sendEthernet(networkData.socket, pingPacket, networkData.ethernetDropRate);
        if(sentonMan || sentonEth){
            if(waitForACK(pingPacket)){
                recived = true;
                gettimeofday(&end, NULL);
                // compute the elapsed time in millisec
                elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;      // sec to ms
                elapsedTime += (end.tv_usec - start.tv_usec) / 1000.0;   // us to ms
                packetsPerSecond = 1000/elapsedTime;
                logData("log.txt", pingPacket, "ACK", "Ethernet & MANETs");
            }
            else
                counter++;
        }
        else{
            counter ++;
        }
    }
    if(!recived){
        logData("log.txt", pingPacket, "NACK", "Ethernet & MANETs");
        printf(" ops, seems both connection are down!!!!\n");
    }
    return packetsPerSecond;
}

void floodingRouting(int sock, struct node myInfo,struct packet buffPacket){
    struct hostent *host;
    struct sockaddr_in manets_addr;
    bool sent = false;
    long buffSize = sizeof(struct packet);
    if(links>0){
        for(int i=0;i<links;i++){
            //setup the Ethernet UDP socket
            manets_addr.sin_family = AF_INET;
            manets_addr.sin_port = htons(connectedNodes[i].port);
            host = (struct hostent *) gethostbyname(connectedNodes[i].name);
            manets_addr.sin_addr = *((struct in_addr *)host->h_addr);
            bzero(&(manets_addr.sin_zero),8);
            if(buffPacket.previous == connectedNodes[i].id){
                printf("sendback Prevented !\n");
            }
            else {
                float dropRate = calculateDropRate(myInfo,connectedNodes[i], networkData.range);
                buffPacket.previous = myInfo.id;
                bool sent = false;
                int tries = 0;
                while (!sent && tries < 5){
                    if(gremlin(dropRate)){
                        printf("packet %d dropped from node %d!!\n",buffPacket.seq, connectedNodes[i].id);
                        tries++;
                    }
                    else{
                        int n = sendto(sock, &buffPacket,buffSize, 0,(struct sockaddr *)&manets_addr, sizeof(struct sockaddr_in));
                        if(n>0){
                            printf("packet %d forwarded to node %d\n",buffPacket.seq, connectedNodes[i].id);
                            sent = true;
                        }
                    }
                }
                if(!sent){
                    printf(" node %d is out of range!!\n", connectedNodes[i].id);
                }
            }
        }
        //  printf("Node Type: %d \n ", type);
        //  printf("Packet Type: %c\n", buffPacket.type[0]);
        if(buffPacket.type[0] == 'D' && sent == true)
            latestSeq[buffPacket.source] = buffPacket.seq;
        else if (buffPacket.type[0] == 'A' && sent == true)
            lastDataAck[buffPacket.source] = buffPacket.seq;
       // printf("Latest ACK recived: %d \n", lastAck);
       // printf("Latest Data recived: %d \n", latestSeq);
    }
}

void sendData(int seqNo, struct node serverInfo, struct node myInfo){
    struct packet buffPacket;
    float networkBandwidth = 0;
    float ethSpeed = 0;
    float manSpeed = 0;
    float speedRate = networkData.ethernetBandidth / (networkData.ethernetBandidth + networkData.manetsBandidth);
    float ethernetRate = 1- (networkData.ethernetDropRate / (networkData.ethernetDropRate + networkData.manetsDropRate));
    float pEthernet = ethernetRate + speedRate /2;
    //reseting the sequence number if the number gets too large
    
    //seting up the packet
    buffPacket.type[0] = 'D';
    buffPacket.dist = serverInfo.id;
    buffPacket.source = myInfo.id;
    buffPacket.previous = myInfo.id;
    buffPacket.seq = seqNo;
    buffPacket.oxygenLevel = sensors.oxygenLevel;
    buffPacket.heartRate = sensors.heartRate;
    buffPacket.latitude = sensors.latitude;
    buffPacket.longitude = sensors.longitude;
    buffPacket.toxicgas = sensors.toxicGas;
    buffPacket.isCritical = isCritical(sensors.heartRate, sensors.oxygenLevel);
    //Drop one of the networks randomely
    if(gremlin(0.3)){
        if(gremlin(0.5)){
            printf("  -- Ethernet connection just went down !\n\n");
            pEthernet = 0;
        }
        else{
            printf("  -- MANETs connection just went down !\n\n");
            pEthernet = 1;
        }
    }
    else{
        printf("  -- Both connections are up\n\n");
    }
    // if the packet is critical we send it to both networks at the same time
    if(buffPacket.isCritical == true){
        printf("packet %d is Critical Sending on both connections! \n",seqNo);
        networkBandwidth += pingBothConnection(buffPacket);
        networkBandwidth = networkBandwidth / 2;
        networkData.ethernetBandidth= (networkData.ethernetBandidth + networkBandwidth) / 2;
        networkData.manetsBandidth= (networkData.manetsBandidth + networkBandwidth) / 2;
        printf("  -- Total Parallel flow bandwidth: %f p/s \n\n", networkBandwidth);
    }
    else{
        if (gremlin(ethernetRate)){
            ethSpeed = pingEtherNet(buffPacket);
            networkData.ethernetBandidth= (networkData.ethernetBandidth + ethSpeed) / 2;
            printf("  -- Ethernet bandwidth: %f p/s \n\n", networkData.ethernetBandidth);
            if (ethSpeed<1){
                printf("X Ethernet was down Trying Mannets! \n");
                manSpeed = pingMANETs(buffPacket);
                networkData.manetsBandidth= (networkData.manetsBandidth + manSpeed) / 2;
                printf("  -- MANETs bandwidth: %f p/s \n\n", networkData.manetsBandidth);
            }
            //getting the time of the send requset
        }
        else {
            manSpeed = pingMANETs(buffPacket);
            networkData.manetsBandidth= (networkData.manetsBandidth + manSpeed) / 2;
            printf("  -- MANETs bandwidth: %f p/s \n\n", networkData.manetsBandidth);
            if(manSpeed <1){
                printf("X Manets was down Trying Ethernet! \n");
                ethSpeed = pingEtherNet(buffPacket);
                networkData.ethernetBandidth= (networkData.ethernetBandidth + ethSpeed) / 2;
                printf("  -- Ethernet bandwidth: %f p/s \n\n", networkData.ethernetBandidth);
            }
        }
    }
    
    // drop ethernet connection randomly
    // fflush(stdout);
    
}
void updateHeardFromTable(struct packet buffPacket){
    bool exist = false;
    int i;
    if(heardFromN>0){
        for (i=0; i<heardFromN; i++){
            if(heardFrom[i] == buffPacket.source){
                exist = true;
                break;
            }
        }
    }
    if(exist == false){
        heardFrom[heardFromN] = buffPacket.source;
        heardFromN++;
    }
}
void updateOneHopTable(struct packet buffPacket, struct node myInfo){
    bool exist = false;
    int i;
    struct timeval t1;
    if(oneHopN>0){
        for(i=0;i<oneHopN;i++){
            if (oneHopNeighbors[i].id == buffPacket.source){
                exist = true;
                oneHopNeighbors[i].sequence = buffPacket.seq;
                gettimeofday(&t1, NULL);
                oneHopNeighbors[i].updateTime = t1;
                oneHopNeighbors[i].state[0] = 'U';
                break;
            }
        }
    }
    if (exist == false){
        oneHopNeighbors[oneHopN].id = buffPacket.source;
        oneHopNeighbors[oneHopN].sequence = buffPacket.seq;
        gettimeofday(&t1, NULL);
        oneHopNeighbors[oneHopN].updateTime = t1;
        oneHopNeighbors[oneHopN].state[0] = 'U';
        oneHopN++;
    }
}

bool isNodeInOneHopTable(short nodeID){
    int i;
    for(i=0;i<oneHopN;i++){
        if(nodeID == oneHopNeighbors[i].id)
            return true;
    }
    return false;
}
bool isNodesInTwoHopTable(short nodeID, short accessFrom){
    int i;
    for (i=0;i<twoHopN; i++){
        if(twoHopNeighbors[i].access == accessFrom && twoHopNeighbors[i].id == nodeID)
            return true;
    }
    return false;
}
void updateTwoHopTable(int listDigits,short source, struct node myInfo, int base){
    short tempList[MAX_NODES];
    int i;
    int size = floatToArray(tempList, listDigits, base);
    for(i=0; i<size;i++){
        if(tempList[i] != myInfo.id && isNodeInOneHopTable(tempList[i])==false && isNodesInTwoHopTable(tempList[i], source)==false){
            twoHopNeighbors[twoHopN].access = source;
            twoHopNeighbors[twoHopN].id = tempList[i];
            twoHopN++;
        }
    }
    
}

/*
 The number of one hope nodes is oneHopN
 The number of links in two hop nodes is twoHopN
 
 
 */
void generateState(struct oneHop *oneHopTable,struct twoHop *twoHopTable){
    int i,j,mult=0,m,n=0,x=0,passIndex=0,passRecord=0,passCheck=0;
    short memory[MAX_NODES],pass[100];
    int len=0;
    
    for(i=0;i<twoHopN;i++){//check repetition
        memset(memory,0,sizeof(memory));
        for(passRecord=0;passRecord<100;passRecord++){
            if(twoHopTable[i].id==pass[passRecord]){
                passCheck=1;
                break;
            }
        }
        if(passCheck==1){
            passCheck=0;
            continue;
        }
        for(j=0;j<twoHopN;j++){
            if((twoHopTable[i].id)==(twoHopTable[j].id)){
                mult++;
                memory[n]=twoHopTable[j].access;
                n++;
                pass[passIndex]=twoHopTable[i].id;
                passIndex++;
            }
        }
        n=0;
        if(mult==1){//if there is only one access, making it to MPR
            for(m=0;m<MAX_NODES;m++){
                if((twoHopTable[i].access)==(oneHopTable[m].id)){
                    oneHopTable[m].state[0]='M';
                }
            }
            mult=0;
        }else{//if there is a multiple degrees node, finding the most degrees of node as MPR
            len=0;
            while(memory[len++] !=0);
            int count=0;
            short findMPR[10];
            memset(findMPR,0,sizeof(findMPR));
            for(m=0;m<len-1;m++){
                for(n=0;n<twoHopN;n++){
                    if((memory[m])==twoHopTable[n].access){
                        count++;
                    }
                }
                findMPR[m]=count;
                count=0;
            }
            n=0;
            short max=findMPR[0],maxindx=0;
            len=0;
            while(findMPR[len++] !=0);
            for(x=0;x<len-1;x++){
                if(findMPR[x]>max){
                    max=findMPR[x];
                    maxindx=x;
                }
            }
            int MultMPR=memory[maxindx];
            mult=0;
            for(m=0;m<oneHopN;m++){
                if((oneHopTable[m].id)==MultMPR){
                    oneHopTable[m].state[0]='M';
                    break;
                }
            }
        }
    }
    /*
     //print for test
     for(i=0;i<10;i++){
     printf("%d",oneHopTable[i].id);
     printf("-");
     printf(oneHopTable[i].status);
     printf("\n");
     }*/
}
/*Updating the tables
 1: heardFrom table (just id list )
 those nodes that this node recived hello packets from
 2: oneHopeNeighbors table (struct of oneHope types uni-directional 'U', bi-directional 'B', MPR 'M' )
 one hop neighbors that recived backet from only are stated with uni direction
 when finding my node id inside the hearing from this node become bi-directional
 algorthim is run to select MPR nodes and changing their stats to 'M'
 3: TwoHopeNeighborrs table (struct of twoHope nodes id and id of the node to access )
 those nodes that are heard from a neighboring node
 4: MPRSelectors table (just id list)
 those nodes that selected this node as their MPR node
 */
bool updateTables(struct packet buffPacket, struct node myInfo){
    short tempList[MAX_NODES];
    bzero(&(tempList), MAX_NODES * sizeof(short));
    int i, j;
    int size =0;
    bool exist = false;
    //1: updating heard from table
    updateHeardFromTable(buffPacket);
    //2: updating the oneHopNeighbor table
    updateOneHopTable(buffPacket, myInfo);
    
    //3: towHopeNeighbors
    // updating the twoHop Table
    if(buffPacket.latitude > 0){
        updateTwoHopTable(buffPacket.latitude,buffPacket.source, myInfo, 10);
    }
    
    if (buffPacket.longitude > 0){
        updateTwoHopTable(buffPacket.longitude,buffPacket.source, myInfo, 100);
    }
    bzero(&(tempList), MAX_NODES * sizeof(short));
    // bi-directional connection from Heard From list
    if(buffPacket.heartRate >0 && myInfo.id < 10){
        size = floatToArray(tempList, (int) buffPacket.latitude, 10);
        if(size>0){
            for(i=0; i<size;i++){
                if(tempList[i] == myInfo.id){
                    for(j=0; j<oneHopN;j++){
                        if(oneHopNeighbors[j].id == buffPacket.source){
                            oneHopNeighbors[j].state[0] = 'B';
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    if (buffPacket.oxygenLevel > 0 && myInfo.id >=10){
        size = floatToArray(tempList, (int) buffPacket.latitude, 100);
        if(size>0){
            for(i=0; i<size;i++){
                if(tempList[i] == myInfo.id){
                    for(j=0; j<oneHopN;j++){
                        if(oneHopNeighbors[j].id == buffPacket.source){
                            oneHopNeighbors[j].state[0] = 'B';
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    bzero(&(tempList), MAX_NODES * sizeof(short));
    //4. MPR lists find if my node is selected as MPR
    if(buffPacket.toxicgas > 0 && myInfo.id >=10){
        size = floatToArray(tempList, (int) buffPacket.latitude, 10);
        if(size>0){
            for(i=0;i<size;i++){
                if(tempList[i] == myInfo.id){
                    for(j=0;j<MPRSelectorsN;j++){
                        if(MPRSelectors[j] ==buffPacket.source){
                            exist = true;
                            break;
                        }
                    }
                    if(exist == false){
                        MPRSelectors[MPRSelectorsN] = buffPacket.source;
                    }
                    break;
                }
            }
        }
    }
    if (buffPacket.isCritical > 0 && myInfo.id >=10){
        size = floatToArray(tempList, (int) buffPacket.latitude, 100);
        if(size>0){
            for(i=0;i<size;i++){
                if(tempList[i] == myInfo.id){
                    for(j=0;j<MPRSelectorsN;j++){
                        if(MPRSelectors[j] ==buffPacket.source){
                            exist = true;
                            break;
                        }
                    }
                    if(exist == false){
                        MPRSelectors[MPRSelectorsN] = buffPacket.source;
                    }
                    break;
                }
            }
        }
    }
    generateState(oneHopNeighbors,twoHopNeighbors);
    return true;
}
//sensing if the node is one of the selector node or not
bool isNodeMPRSelector(short NodeID){
    int i;
    for (i=0;i<MPRSelectorsN;i++){
        if(MPRSelectors[i] == NodeID)
            return true;
    }
    return false;
}
// sensing if the connection is bi-directional or not. the node see its node id in the list
bool isNodeBiDirectional(short list[], struct node myInfo){
    int i;
    for (i=0;i<MAX_NODES;i++){
        if (list[i] == myInfo.id){
            return true;
        }
    }
    return false;
}
// sensing if the node selected me as one of its MPR or not. the node see its node id in the list
bool didThisNodeSelectedMeAsMPR(short list[],struct node myInfo){
    int i;
    for (i=0;i<MAX_NODES;i++){
        if (list[i] == myInfo.id){
            return true;
        }
    }
    return false;
}
//return selected list from oneHop Neighbors
int selectList(short selectedList[], int max, int min){
    int size = 0;
    int i;
    for (i=0; i<oneHopN; i++){
        if(oneHopNeighbors[i].id <= max && oneHopNeighbors[i].id >= min){
            selectedList[size] = oneHopNeighbors[i].id;
            size++;
        }
    }
    return size;
}
//return selected list from oneHop Neighbors
int selectListHeardFrom(short selectedList[], int max, int min){
    int size = 0;
    int i;
    for (i=0; i<heardFromN; i++){
        if(heardFrom[i] <= max && heardFrom[i] >= min){
            selectedList[size] = heardFrom[i];
            size++;
        }
    }
    return size;
}
int findMPRList(short selectedList[], int max, int min){
    int size = 0;
    int i;
    for (i=0; i<oneHopN; i++){
        if(oneHopNeighbors[i].state[0] == 'M' && oneHopNeighbors[i].id < max && oneHopNeighbors[i].id > min){
            selectedList[size] = oneHopNeighbors[i].id;
            size++;
        }
    }
    return size;
}
struct packet setupHelloMessage(){
    int size;
    struct packet buffPacket;
    short selectedList[MAX_NODES];
    bzero(&(selectedList),MAX_NODES * sizeof(short));
    //one hop neighbors list
    size = selectList(selectedList,9, 1);
    buffPacket.latitude = (float) arrayToFloat(selectedList, 10);
    bzero(&(selectedList),MAX_NODES * sizeof(short));
    size = selectList(selectedList,99, 10);
    buffPacket.longitude = (float) arrayToFloat(selectedList, 100);
    bzero(&(selectedList),MAX_NODES * sizeof(short));
    // heard from list
    size = selectListHeardFrom(selectedList,9, 1);
    buffPacket.heartRate = (float) arrayToFloat(selectedList, 10);
    bzero(&(selectedList),MAX_NODES * sizeof(short));
    size = selectListHeardFrom(selectedList,99, 10);
    buffPacket.oxygenLevel = (float) arrayToFloat(selectedList, 100);
    bzero(&(selectedList),MAX_NODES * sizeof(short));
    //MPR list
    size = findMPRList(selectedList,9, 1);
    buffPacket.toxicgas = (float) arrayToFloat(selectedList, 10);
    bzero(&(selectedList),MAX_NODES * sizeof(short));
    size = findMPRList(selectedList,99, 10);
    buffPacket.isCritical = (float) arrayToFloat(selectedList, 100);
    bzero(&(selectedList),MAX_NODES * sizeof(short));
    
    return buffPacket;
}

void getSettingsFromFile(const char *fileName){
    char *lineBuffer= NULL;
    size_t length = 200;
    ssize_t bytesRead;
    FILE *fp;
    fp = fopen(fileName, "r");
    //printf("file Opened \n");
    bytesRead =getline(&lineBuffer, &length, fp);
    sscanf(lineBuffer, "Ethernet: %f", &networkData.ethernetDropRate);
    bytesRead =getline(&lineBuffer, &length, fp);
    sscanf(lineBuffer, "Manets: %f", &networkData.manetsDropRate);
    //printf("found node: %d \n", readNode.id);
    fclose(fp);
}

void *sendHelloMessagesThread(){
    struct packet buffPacket;
    struct node nodeInfo;
    int i;
    int sequence = 0;
    struct timespec time;
    time.tv_sec = 3;
    time.tv_nsec = 0;
    while(1){
        if (sequence >9999)
            sequence = 0;
        sequence++;
        buffPacket = setupHelloMessage();
        buffPacket.type[0] = 'H';
        buffPacket.source = myInfo.id;
        buffPacket.previous = myInfo.id;
        buffPacket.seq = sequence;
        for(i=0;i<links;i++){
            buffPacket.dist = connectedNodes[i].id;
            nodeInfo = connectedNodes[i];
            printf("*** Hello message: ");
            sendPacket(networkData.socket, buffPacket, nodeInfo, 0);
        }
        printf("\n");
        nanosleep(&time, (struct timespec *)NULL);
    }
    return NULL;
}

bool isTargetInOneNeighborsList(short target){
    int i;
    for(i=0;i<oneHopN; i++){
        if(oneHopNeighbors[i].id == target)
            return true;
    }
    return false;
}
//OLSR rounting to MPR nodes, if the target in the one neighbor range send to target directly.
void routPacketOLSR(struct packet buffer){
    struct packet buffPacket;
    float dropRate;
    buffPacket = buffer;
    struct node nodeInfo;
    bool status = false;
    int count = 0;
    int MPRCount = 0;
    int i;
    short previousHop = buffPacket.previous;
    buffPacket.previous = myInfo.id;
    if(oneHopN >0){
        
        if(isTargetInOneNeighborsList(buffPacket.dist)){
            nodeInfo = findNodeFromConnectedList(buffPacket.dist);
            if(nodeInfo.id >0){
                dropRate = calculateDropRate(myInfo, nodeInfo, networkData.range);
                printf("* Sending Packet to distination: ");
                while(status == false && count < 3){
                    status = sendPacket(networkData.socket, buffPacket, nodeInfo, dropRate);
                    count++;
                    MPRCount++;
                }
            }
            else{
                printf("X couldn't find node %hd in the connection list !\n", oneHopNeighbors[i].id );
            }
        }
        else{
            for(i=0;i<oneHopN;i++){
                if(oneHopNeighbors[i].state[0] =='M' && oneHopNeighbors[i].id >0 && oneHopNeighbors[i].id != previousHop){
                    nodeInfo = findNodeFromConnectedList(oneHopNeighbors[i].id);
                    if(nodeInfo.id >0){
                        
                        dropRate = calculateDropRate(myInfo, nodeInfo, networkData.range);
                        printf("* Routing Packet: ");
                        while(status == false && count < 3){
                            status = sendPacket(networkData.socket, buffPacket, nodeInfo, dropRate);
                            if(status == true){
                                MPRCount++;
                            }
                            count++;
                        }
                    }
                    else{
                        printf("X couldn't find node %hd in the connection list !\n", oneHopNeighbors[i].id );
                    }
                }
                if(oneHopNeighbors[i].state[0] =='M' && oneHopNeighbors[i].id >0 && oneHopNeighbors[i].id == previousHop){
                    printf("* SendBack is prevented.\n");
                }
                count =0;
                status = false;
            }
        }
    }
    else{
        if(buffPacket.type[0] =='A'){
            lastDataAck[buffPacket.source]--;
        }
        else if(buffPacket.type[0] == 'D'){
            latestSeq[buffPacket.source]--;
        }
        printf("X Seems i am disconnected from other nodes !! forwarding packets faild !\n");
    }
    if(MPRCount == 0){
        generateState(oneHopNeighbors, twoHopNeighbors);
        if(buffPacket.type[0] =='A'){
            lastDataAck[buffPacket.source]--;
        }
        else if(buffPacket.type[0] == 'D'){
            latestSeq[buffPacket.source]--;
        }
        printf("X There were no MPR nodes !! forwarding packets faild !\n");
    }
}
void *sendDataThread(){
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 70000000; //70 ms delay to insure sensors reading are updated correctly by the other threads
    int seqNo = 0;
    //setting up the sensors data. asuming there were no data inside before.
    struct sensorsData previousSensorsReadings;
    previousSensorsReadings.heartRate = 0;
    previousSensorsReadings.toxicGas = 0;
    previousSensorsReadings.latitude =0;
    previousSensorsReadings.longitude = 0;
    previousSensorsReadings.oxygenLevel = 0;
    //initial speed asummed that both connection can do up to 50 packets per seconds
    networkData.ethernetBandidth = 50;
    networkData.ethernetBandidth = 50;
    while(1){
        if(seqNo > 9999)
            seqNo = 0;
        if (isSensorDataChanged(previousSensorsReadings, sensors)){
            seqNo++;
            previousSensorsReadings = sensors;
            sendData(seqNo,  serverInfo, myInfo);
        }
        nanosleep(&time, (struct timespec *)NULL);
    }
    return NULL;
}

void deleteLinksbySource(short sourceID){
    int i, j;
    if(twoHopN > 0){
        for(j=twoHopN;j>0;j--){
            for(i=0;i<twoHopN;i++){
                if(twoHopNeighbors[i].access == sourceID){
                    twoHopNeighbors[i] = twoHopNeighbors[twoHopN-1];
                    twoHopN--;
                }
            }
        }
    }
}

void deleteHeardFromByID(short sourceID){
    int i;
    if(heardFromN>0){
        for(i=0; i< heardFromN;i++){
            if(sourceID == heardFrom[i]){
                heardFrom[i] = heardFrom[heardFromN-1];
                //heardFrom[heardFromN] = 0;
                heardFromN--;
            }
        }
    }
}
void deleteOutDatedNeighbbors(double timeout){
    int i;
    struct timeval t1, t2;
    short sourceID;
    double elapsedTime = 0;
    if(oneHopN>0){
        for(i =0; i<oneHopN; i++){
            t1 = oneHopNeighbors[i].updateTime;
            gettimeofday(&t2, NULL);
            
            // compute the elapsed time in millisec
            elapsedTime = (t2.tv_sec - t1.tv_sec) ;      // in seconds
            // elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;   // us to sec
            if(elapsedTime > timeout){
                sourceID = oneHopNeighbors[i].id;
                printf("X Node %hd is outdated, deleting all related links!!\n", sourceID);
                oneHopNeighbors[i]= oneHopNeighbors[oneHopN-1];
                oneHopN--;
                deleteLinksbySource(sourceID);
                deleteHeardFromByID(sourceID);
            }
        }
    }
}

void *deleteOutDatedNeighbborsThread(){
    struct timespec time;
    time.tv_sec = 6;
    time.tv_nsec = 0;
    while(1){
        nanosleep(&time, (struct timespec *)NULL);
        deleteOutDatedNeighbbors(5.0);
    }
    return NULL;
}

