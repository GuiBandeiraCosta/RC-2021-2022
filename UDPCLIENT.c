#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#define PORT_DEF 58011

char dsip[30];
char dsport[8];
int dsport_err;



/* Connection*/
int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
/*end*/

int InputParse(int argc,char* argv[]){
    char *error;
    if(argc == 1){
        gethostname(dsip,20);
        dsport_err = PORT_DEF;
    }
    else if((argc == 3) && (strcmp(argv[1],"-n") == 0||strcmp(argv[1],"-p") == 0)){
        if(strcmp(argv[1],"-n") == 0){
            strcpy(dsip,argv[2]);
            dsport_err = PORT_DEF;
        }
        else if(strcmp(argv[1],"-p") == 0){
            gethostname(dsip,20);
            dsport_err = strtol(argv[2],&error,10);
            if((dsport_err == 0) && (strlen(error) != 0)) {
                printf("Invalid PORT\n"); 
                exit(1);
            }
       } 
    }
    else if((argc == 5) && (strcmp(argv[1],"-n") == 0) && (strcmp(argv[3],"-p") == 0)){
        strcpy(dsip,argv[2]);
        dsport_err = strtol(argv[4],&error,10);
        if((dsport_err == 0) && (strlen(error) != 0)) {
                printf("Invalid PORT\n"); 
                exit(1);
        }
    }
    else{
        printf("Wrong format\n");
        exit(1);
    }
}


int main(int argc,char* argv[]){
    InputParse(argc, argv);
    sprintf(dsport, "%d",dsport_err);
    printf(" ID %s  PORT %s\n",dsip,dsport);
    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if(fd==-1) exit(1);
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM; //UDP socket
    errcode=getaddrinfo(dsip,dsport,&hints,&res);
    
    if(errcode!=0) /*error*/ exit(1);
    while(1){
        char input[30];
        char command[13];
        fgets(input,30,stdin);
        sscanf(input,"%s",command);
        if(strcmp(command,"reg")== 0){
            char send[20];
            char uid_str[6];
            char password[9];
            
            sscanf(input,"%s %s %s",command,uid_str,password);
            
            
            if(strlen(uid_str)!=5){ 
                printf("Invalid UID: Must be 5 digits long\n");
                break;
            }
            
            else if(strlen(password) !=8){
                printf("Password must be 8 digits long\n");
                break;
            }
            sprintf(send,"REG %s %s\n",uid_str,password);
            n=sendto(fd,send,19,0,res->ai_addr,res->ai_addrlen);
            if(n==-1)  exit(1);
            addrlen=sizeof(addr);
            n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
            if(n==-1)  exit(1);
            printf("|%s|",buffer);
            if(strcmp(buffer,"RRG OK\n") == 0){ /*Pq recebemos dois \n\n*/
                printf("User successfully registered\n");
            }
            else if(strcmp(buffer,"RRG DUP\n") == 0){
                printf("User already registered\n");
            }
            else if(strcmp(buffer,"RRG NOK\n") == 0){
                printf("Registration failed\n");
            }
            else if(strcmp(buffer,"ERR\n") == 0){
                printf("ERROR:unexpected protocol message\n");
            }
            else{
                printf("Something went wrong,try again\n");
            }  
        }
        
    }

    freeaddrinfo(res);
    close(fd);

   
    return 0 ;
}
    
