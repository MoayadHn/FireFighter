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

typedef int bool;
#define true 1
#define false 0


/********************************  Functions decleration  ***********************************/

/*********************************** Main function ***************************************/
int main(int argc, char *argv[]){
    
    //variables
    int numberOfFiles=0;
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 500 * 1000000L;
    int filecounter;
    size_t length =0 ;
    FILE *fp;
    //seting up the file name
    char baseFileName[10];
    char fileName[30];
    char configFile[50];
    char *lineBuffer = NULL;
    char fileBuffer[MAX_SIZE];
    sprintf(baseFileName, "node");
    bool dataRead = false;
    strcat(fileName,baseFileName);
    //    // Getting the program arguments and managing errors in this part
    if (argc <5 ){
        fprintf(stderr,"ERROR, Usage:%s -f <configuration file> -n <number of nodes> \n",argv[0]);
        exit(1);
    }
    int in;
    for(in=1;in<4;in+=2){
    if (strncmp(argv[in],"-n",2)==0){
        numberOfFiles =  atoi(argv[in+1]);
    }
    else if (strncmp(argv[in],"-f",2)==0){
        strcpy(configFile, argv[in+1]);
    }
    else {
        fprintf(stderr,"ERROR, Wrong argument type '%s'\n",argv[1]);
        fprintf(stderr,"ERROR, Usage:%s -f <configuration file> -n <number of nodes> \n",argv[0]);
        exit(1);
    }
    }
    printf("...monitornig files ... \n");
    while(1){
        dataRead = false;
        nanosleep(&time, (struct timespec *)NULL);
        // read every file
        for(filecounter=1;filecounter<=numberOfFiles;filecounter++){
            sprintf(fileName, "%s%d.txt",baseFileName ,filecounter);
            //strcat(fileName,baseFileName);
            if((fp = fopen(fileName,"r"))>0){
                getline(&lineBuffer, &length, fp);
                strcat(fileBuffer,lineBuffer);
                dataRead = true;
                fclose(fp);
            }
        }
        if(dataRead == true){
            fp = fopen(configFile, "w");
            fprintf(fp, "%s", fileBuffer);
            fclose(fp);
        }
        bzero(fileBuffer, MAX_SIZE);
        //Write the main configuration file
    }
    return 0;
}

/***************************  Functions Defnitions **********************************/
