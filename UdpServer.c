#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
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
int flag_v = 0;
int dsport_err;
int n_clients = 0;

/*Connection*/
int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
/*end*/

int CreateUserDir(char UID[],char password[]){
    char user_dirname[20];
    char user_password[30];
    FILE *f;
    int ret;
    sprintf(user_dirname,"USERS/%s",UID);
    ret=mkdir(user_dirname,0700);
    if(ret==-1) return(0);
    sprintf(user_password,"%s/pass.txt",user_dirname);
    f = fopen(user_password,"w");
    if(f == NULL) return 0;
    fputs(password,f);
    fclose(f);
    return(1);
}

int SearchUID(char uid[]){
    DIR *d;
    struct dirent *dir;
    int i = 0;
    d = opendir("USERS");
    if (d){
        while ((dir = readdir(d)) != NULL){
           if (strcmp(dir->d_name, uid) == 0){
               closedir(d);
               return 1;
            }
        }
    }
    else{
        closedir(d);
        return 0;
    }
}

int InputParse(int argc, char*argv[]){
    char *error;
    if(argc == 1){
        dsport_err = PORT_DEF;
    }
    else if((argc == 2) &&  (strcmp(argv[1],"-v") == 0)){
        flag_v = 1;
        dsport_err = PORT_DEF;
    }
    else if((argc == 3) && (strcmp(argv[1],"-p") == 0)){
        dsport_err = strtol(argv[2],&error,10);
        if((dsport_err == 0) && (strlen(error) != 0)) {
            printf("Invalid PORT\n"); 
            exit(1);
        }
    }
    else if((argc == 4) && (strcmp(argv[1],"-p") == 0) && (strcmp(argv[3],"-v") == 0)){
        dsport_err = strtol(argv[2],&error,10);
        if((dsport_err == 0) && (strlen(error) != 0)) {
            printf("Invalid PORT\n"); 
            exit(1);
        }
        flag_v = 1;
    }
    else{
        printf("Wrong Format\n");
        exit(1);
    }
}

int main(int argc, char *argv[]){
    InputParse(argc,argv);
    int ret;
    ret=mkdir("USERS",0700);
    ret=mkdir("GROUPS",0700);
    
    /*Connection*/
    fd= socket(AF_INET,SOCK_DGRAM,0);
    if(fd ==-1) exit(1);
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE;
    sprintf(dsport, "%d",dsport_err);
    errcode=getaddrinfo(NULL,dsport,&hints,&res);
    if(errcode != 0) exit(1);

    n= bind(fd,res->ai_addr,res->ai_addrlen);
    if(n==-1) exit(1);
    while(1){
        addrlen = sizeof(addr);
        char command[13];
        n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
        if(n==-1) exit(1);
        printf("isto e o buffer %s\n",buffer);
        sscanf(buffer,"%s",command);
        /*REG*/
        if(strcmp(command,"REG")== 0){
            
            char uid_str[6];
            char password[9];
            sscanf(buffer,"%s %s %s",command,uid_str,password);
            
            if(strlen(uid_str)!=5 || (strlen(password) !=8) || (n_clients >= 100000)){ /*ERROR*/
                 n = sendto(fd,"RRG NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                 if(n==-1) exit(1);
                 printf("Fiz isto1\n");
            }
            else if(SearchUID(uid_str) != 0){
                 n = sendto(fd,"RRG DUP\n",n,0,(struct sockaddr*)&addr,addrlen);
                 if(n==-1) exit(1);
                 printf("Fiz isto2\n");
            }
            else{
                if (CreateUserDir(uid_str,password) == 0){
                    n = sendto(fd,"RRG NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                else{
                    n = sendto(fd,"RRG OK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                    printf("Fiz isto3\n");
                }  
            }   
        }
    }
    /*end*/
    freeaddrinfo(res);
    close(fd);

    printf("DSPORT %d Flag %d\n",dsport_err,flag_v);
    
    return 0;
    
}