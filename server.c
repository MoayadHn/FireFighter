
/* server.c    server take the arguments -p <port> Optional -d <dataCount>   */

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
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/file.h>

//Defining max size for the file buffer
#define MAX_SIZE 51200
#define MAX_NODES 50

//defining bool variable it will be int but easier to use true and false than 0 and 1
typedef int bool;
#define true 1
#define false 0

// function for error handling
void error(const char *msg){
    perror(msg);
    exit(1);
}
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
    float EthernetDropRate;
    float manetsDropRate;
    int socket;
};

//global variables
struct sensorsData sensors;
struct networkInfo networkData;
struct node myInfo;
struct node connectedNodes[MAX_NODES];
int links =0;
/****   Functions  ****/
bool gremlin(float rate);
void writeBaseInfoToFile(const char *fileName, struct node myInfo);
void findConnectedNodes(const char *fileName,struct node myInfo, float radius);
int getLocationFromFile(const char *fileName,int nodeID);
float getDistance(struct node myInfo, struct node nextNode);
bool isNodeInRadious(struct node myInfo, struct node nextNode, float radius);
float getLongitude(float base);
float getLatitude(float base);
bool isSensorLocationChanged(struct node myInfo, struct sensorsData newReadings);
float depleatRange(float base);
void *locationThread(void *result);
void *depleatRangelThread(void *result);
/*********************************** Main function ***************************************/
int main(int argc, char *argv[]){
    // variables
    FILE *fp;
    int sock;
    int dataCount = 1; // how many data packets to to avrage then store as single reading
    int count = 1; // packets counter
    int waitTime = 40000; //set timeout for 40 ms
    unsigned int addr_len;
    struct sockaddr_in server_addr, client_addr, my_addr;
    int portno= -1;
    int buffSize;
    int r=0;
    bool depleatRange;
    bool readConfigFileFirst = false;
    char configFile[50];
    //variables for avraging data
    int HR = 0;
    int sumHR = 0;
    float O2 = 0;
    float sumO2 = 0;
    float toxic = 0;
    float sumToxic = 0;
    //variables time
    struct tm *tm;
    struct timeval t0;
    //file and line buffers to store to the file
    char fileBuffer[MAX_SIZE];
    char lineBuffer[200];
    char packetSource[20];
    // packet structure, we are using structure for our UDP payload data
    // Type ( 'D' : data, 'A': ACK, 'N':NAK )
    typedef struct packet{
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
    } packet;
    
    packet buffpacket;
    
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    printf("Hostname: %s\n", hostname);
    struct hostent* myName;
    short source;
    myName = gethostbyname(hostname);
    strcpy(myInfo.name, hostname);
    printf("--------------------------------------------------\n");
    printf("|            Server: FireFighter Sheif             |\n");
    printf("--------------------------------------------------\n");
    sensors.latitude = getLatitude(32.60618);
    sensors.longitude = getLatitude(-85.48703);

    // Reading the program arguments if not as specified return error message explaining how to do it.
    if (argc <7 ){
        fprintf(stderr,"ERROR, Usage:%s -f <configuration file> -p <port number> -n <node number> Optional -r <depleatRange(0:false, 1:true)> Optional -c <readFromConfigFileFirst(0:false, 1:true)> Optional -l <comunication range>\n",argv[0]);
        exit(1);
    }
    int in;
    for(in=1;in<6;in+=2){
        if (strncmp(argv[in],"-p",2)==0){
            myInfo.port = atoi(argv[in+1]);
        }
        else if (strncmp(argv[in],"-f",2)==0){
            strcpy(configFile, argv[in+1]);
        }
        else if (strncmp(argv[in],"-n",2)==0){
            myInfo.id = atoi(argv[in+1]);
        }
        else {
            fprintf(stderr,"ERROR, Wrong argument type '%s'\n",argv[in]);
            fprintf(stderr,"ERROR, Usage:%s -f <configuration file> -p <port number> -n <node number> Optional -r <depleatRange(0:false, 1:true)> Optional -c <readFromConfigFileFirst(0:false, 1:true)> Optional -l <comunication range>\n",argv[0]);
            exit(1);
        }
    }
    
    // in case of having optional program arguments
    if(argc > 7){
        for(in=7;in<argc;in+=2){
            
            if (strncmp(argv[in],"-r",2)==0){
                depleatRange = atoi(argv[in+1]);
            }
            else if (strncmp(argv[in],"-c",2)==0){
                readConfigFileFirst = atoi(argv[in+1]);
            }
            else if (strncmp(argv[in],"-count",6)==0){
                count = atoi(argv[in+1]);
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
            else{
                fprintf(stderr,"ERROR, Unknown optional argument type '%s'\n",argv[in]);
                fprintf(stderr,"ERROR, Usage:%s -f <configuration file> -p <port number> -n <node number> Optional -r <depleatRange(0:false, 1:true)> Optional -c <readFromConfigFileFirst(0:false, 1:true)> Optional -l <comunication range> \n",argv[0]);
                exit(1);
            }
        }
    }
    if(readConfigFileFirst){
        myInfo.port = getLocationFromFile(configFile,myInfo.id);
    }
       printf("sever: %hd, port %d\n",myInfo.id, myInfo.port);
    myInfo.latitude = sensors.latitude;
    myInfo.longitude = sensors.longitude;
    char baseFileName[10];
    char fileName[30];
    sprintf(baseFileName, "node");
    sprintf(fileName, "%s%hd.txt",baseFileName ,myInfo.id);
    writeBaseInfoToFile(fileName, myInfo);
    printf("--file info read successfully\n");
    //printing info
    // Defining the UDP packet ( SOCK_DGRAM )
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }
    //setting up the address informaton
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(myInfo.port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero),8);
    
    //seting up my address
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(myInfo.port);
    my_addr.sin_addr = *((struct in_addr *)myName->h_addr);
    bzero(&(my_addr.sin_zero),8);
    
    // Binding the socket with the server address
    if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        perror("Bind");
        exit(1);
    }
    addr_len = sizeof(struct sockaddr);
    
    printf("-- Waiting for requests --\n");
    int latestSeq = 0;
    //Main listing section
    pthread_t locationThreadID, RangeThreadID;
    pthread_create(&locationThreadID, NULL, &locationThread, &sensors);
    if(depleatRange){
        pthread_t RangeThreadID;
        pthread_create(&RangeThreadID, NULL, &depleatRangelThread, &networkData);
    }
    struct timeval timeout = {0,waitTime};
        setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
    while(1){
        if(isSensorLocationChanged(myInfo, sensors)){
            myInfo.latitude = sensors.latitude;
            myInfo.longitude = sensors.longitude;
            //bzero(&(myInfo.links),MAX_NODES);
            //links =0;
            findConnectedNodes(configFile,myInfo, networkData.range);
//            printf("\n **my new location: <%f, %f> ", myInfo.latitude, myInfo.longitude);
//            printf("Connected Nodes:");
//            for (int i = 0; i<links;i++){
//                myInfo.links[i] = connectedNodes[i].id;
//                printf(" %d",myInfo.links[i]);
//            }
//            printf("\n");
            //printf("range: %f\n", networkRange.range);
            writeBaseInfoToFile(fileName, myInfo);
        }
        //waiting for packets to be recived. when that happen print out the information of the packet.
        r = recvfrom(sock,&buffpacket,MAX_SIZE, 0,(struct sockaddr *)&client_addr, &addr_len);
        if (r>0){
        source = buffpacket.source;
        if(buffpacket.type[0] =='H'){
            printf("Recived Hello Message from %hd - ", buffpacket.source);
            if(buffpacket.source >= 10){
                buffpacket.oxygenLevel = (float) buffpacket.source;
                buffpacket.longitude = (float) buffpacket.source;
                buffpacket.isCritical = (int) buffpacket.source;
            }
            else{
                buffpacket.heartRate = (int) buffpacket.source;
                buffpacket.latitude = (float) buffpacket.source;
                buffpacket.toxicgas = (float) buffpacket.source;
            }
            buffpacket.type[0] = 'H';
            buffpacket.dist = buffpacket.source;
            buffpacket.source = myInfo.id;
            buffpacket.previous = myInfo.id;
            buffpacket.seq++;
            
            buffSize = sizeof(buffpacket);
            sendto(sock,&buffpacket,buffSize,0,(struct sockaddr *)&client_addr, sizeof(struct sockaddr_in));
            printf("replay Sent \n");
        }

        char tempName[50];
        char tempService[20];
        getnameinfo((struct sockaddr*)&client_addr, sizeof(struct sockaddr_in),tempName, sizeof(tempName), tempService, sizeof(tempService),0);
        if (buffpacket.type[0]=='D' && (buffpacket.seq > latestSeq || latestSeq > buffpacket.seq + 10) ){
            printf("\n--------------------------  Packet %d recived  from: %hd  ---------------------------\n", buffpacket.seq, buffpacket.previous);
            if (buffpacket.isCritical == false)
                printf("****************************************  Node %hd (Safe)  ****************************************\n", buffpacket.source);
            else
                printf("***************************************  Node %hd (Danger)  ***************************************\n", buffpacket.source);
            printf ("*    Heart Rate       *         Oxygen          *     Toxic Gas      *         Location          *\n");
            printf("*         %3d         *          %.2f %%         *      %.3f %%       *< %.6f, %.6f >*\n", buffpacket.heartRate,buffpacket.oxygenLevel, buffpacket.toxicgas , buffpacket.latitude,buffpacket.longitude);
            printf("*************************************************************************************************\n");
            HR = buffpacket.heartRate;
            O2 = buffpacket.oxygenLevel;
            toxic = buffpacket.toxicgas;
            if(buffpacket.source == buffpacket.previous){
                strcpy(packetSource,"Ethernet");
            }
            else{
                strcpy(packetSource,"MANETs");
            }
            //when this is the firest packet in the count for avraging
            if (count == 1){
                
                //  fill up the header for the file buffer and initialize the sum counters.
                sprintf(lineBuffer, "[\n");
                strcpy(fileBuffer,lineBuffer);
                sumHR = 0;
                sumO2 = 0;
                sumToxic = 0;
            }
            if (count < dataCount){
                sumHR = sumHR + HR;
                sumO2 = sumO2 + O2;
                sumToxic = sumToxic + toxic;
                count++;
            }
            else{
                //when this is the last packet to count for avraging, finish caluclating the information needed to store in the file and finalize the file buffer for final write on the file.
                sumHR = sumHR + HR;
                sumO2 = sumO2 + O2;
                sumToxic = sumToxic + toxic;
                int HRAvg = sumHR / dataCount;
                float O2Avg = sumO2 / dataCount;
                float toxAvg = sumToxic /dataCount;
                sprintf(lineBuffer, "{\n");
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer,"\"name\": \"Node%hd\",\n", buffpacket.source);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer,"\"critical\": %d,\n", buffpacket.isCritical);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer,"\"heart\": %d,\n", HRAvg);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer,"\"oxygen\": %.2f,\n", O2Avg);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer,"\"toxic\": %.2f,\n", toxAvg);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer,"\"source\": \"%s\",\n", packetSource);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer, "\"latitude\": %f,\n", buffpacket.latitude);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer, "\"longitude\": %f\n",buffpacket.longitude);
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer,"}\n");
                strcat(fileBuffer,lineBuffer);
                sprintf(lineBuffer, "]\n");
                strcat(fileBuffer,lineBuffer);
                //Writing the file buffer into the data file. we are trying to minimize the calls for the file stream as much as possible.
                fp = fopen("data.json","w");
                fprintf(fp, "%s", fileBuffer);
                fclose(fp);
                //reset the counter
                count = 1;
            }
            //generating responce packet ACK
            buffpacket.type[0] = 'A';
            buffpacket.dist = buffpacket.source;
            buffpacket.source = myInfo.id;
            buffpacket.previous = myInfo.id;
            latestSeq = buffpacket.seq;
            
            buffSize = sizeof(buffpacket);
            sendto(sock,&buffpacket,buffSize,0,(struct sockaddr *)&client_addr, sizeof(struct sockaddr_in));
            //            }
            //else {
            // sendto(sock,&buffpacket,buffSize,0,(struct sockaddr *)&buffpacket.dist, sizeof(struct sockaddr_in));
            //}
            gettimeofday(&t0,NULL);
            tm = localtime(&t0.tv_sec);
            printf("ACK Packet %d sent at %d:%02d:%02d:%d To: %s:%d\n",buffpacket.seq, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000, tempName, ntohs(client_addr.sin_port));
        }
        //in the case of dublicated packet just send ACK packet back to the sender, dont reread the packet data
        else if (buffpacket.type[0]=='D' && buffpacket.seq == latestSeq){
            //generating responce packet
            buffpacket.type[0] = 'A';
            buffpacket.dist = buffpacket.source;
            buffpacket.source = myInfo.id;
            buffpacket.previous = myInfo.id;
            buffpacket.seq = buffpacket.seq;
            buffSize = sizeof(buffpacket);
            sendto(sock,&buffpacket,buffSize,0,(struct sockaddr *)&client_addr, sizeof(struct sockaddr));
            
            gettimeofday(&t0,NULL);
            tm = localtime(&t0.tv_sec);
            //printf("Packet %d is doublicated \n", buffpacket.seq);
            printf("ACK Packet %d sent at %d:%02d:%02d:%d To: %s:%d\n",buffpacket.seq, tm->tm_hour,tm->tm_min,tm->tm_sec,t0.tv_usec/1000, tempName, ntohs(client_addr.sin_port));
        }
            r= 0;
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
bool gremlin(float rate){
    int tempRate=rate*100;
    int rateMatch=rand()%100;
    if(rateMatch<=tempRate){
        return true;
    }else{
        return false;
    }
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
        while (myInfo.links[link] >0 && link < MAX_NODES){
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

// scan the file to find specific node information.. not implements because we chose to update the node from its own GPS instead or reading its info from the file.
int getLocationFromFile(const char *fileName,int nodeID){
    struct node targetNode, readNode;
    char *lineBuffer= NULL;
    size_t length = 200;
    ssize_t bytesRead;
    FILE *fp;
    fp = fopen(fileName, "r");
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

// Reading the configuration file and finding the nodes in range based on the location and using the isNodeInRadius function
void findConnectedNodes(const char *fileName,struct node myInfo, float radius){
    struct node readNode;
    char *lineBuffer= NULL;
    size_t length = 200;
    ssize_t bytesRead;
    FILE *fp;
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
        printf("Error opening file\n");
        return ;
    }
}
// function to determine if node location is updated or not.
bool isSensorLocationChanged(struct node myInfo, struct sensorsData newReadings){
    if (myInfo.latitude != newReadings.latitude)
        return true;
    if (myInfo.longitude != newReadings.longitude)
        return true;
    return false;
}
void *locationThread(void *result){
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 500 * 1000000L;
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
