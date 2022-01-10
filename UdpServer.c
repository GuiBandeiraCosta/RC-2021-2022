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
#define max(A,B) ((A)>=(B)?(A):(B))

char dsip[30] = "";
char dsport[8] = "";
int flag_v = 0;
int dsport_err;
int n_clients = 0;
int n_groups = 0;
typedef struct GROUPLIST{
    int no_groups;
    char group_no[99][3];
    char group_name[99][25];
    char group_message[99][5];
}GROUPLIST;

/*Connection*/
int udpfd,tcpfd,newfd,errcode = 0;
fd_set rfds;
enum {idle,busy} state;
int maxfd,checker;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints_fd,hints_tcp,*res;
struct sockaddr_in addr;
char buffer[128] = "";
/*end*/


int ListGroupsDir(GROUPLIST *list){
    DIR *d;
    struct dirent *dir;
    int i=0;
    FILE *fp;
    char GIDname[530] = "";
    list->no_groups=0;
    d = opendir("GROUPS");
    if (d){
        while ((dir = readdir(d)) != NULL){
            if(dir->d_name[0]=='.')
                continue;
            if(strlen(dir->d_name)>2)
                continue;
            strcpy(list->group_message[i],"0000");
            strcpy(list->group_no[i], dir->d_name);
            sprintf(GIDname,"GROUPS/%s/%s_name.txt",dir->d_name,dir->d_name);
            fp=fopen(GIDname,"r");
            if(fp)
            {
                fscanf(fp,"%24s",list->group_name[i]);
                fclose(fp);
            }
            ++i;
            if(i==99)
            break;
        }
        list->no_groups=i;
        closedir(d);
    }
    else
        return(-1);
    return(list->no_groups);
}


int CreateUserDir(char UID[],char password[]){
    char user_dirname[20] = "";
    char user_password[30] = "";
    FILE *f;
    int ret;
   
    sprintf(user_dirname,"USERS/%s",UID);
    ret=mkdir(user_dirname,0700);
    sprintf(user_password,"%s/%s_pass.txt",user_dirname,UID);
    f = fopen(user_password,"w");
    if(f == NULL) return 0;
    fputs(password,f);
    fclose(f);
    return(1);
}

int DelUserDir(char UID[]){
    char user_dirname[20] = "";
    sprintf(user_dirname,"USERS/%s",UID);
    if(rmdir(user_dirname)==0) return(1);
    else return(0);
}

int DelPassFile(char UID[]){
    char pathname[50] = "";
    sprintf(pathname,"USERS/%s/%s_pass.txt",UID,UID);
    if(unlink(pathname)==0)
    return(1);
    else
    return(0);
}

int DelLoginFile(char UID[]){
    char pathname[30] = "";
    FILE *f;
    sprintf(pathname,"USERS/%s/%s_login.txt",UID,UID);

    if(access( pathname, F_OK ) == 0){
        
        if(unlink(pathname)==0)
            return(1);
    }
    else
    return(0);
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
    udpfd = socket(AF_INET,SOCK_DGRAM,0) /*UDP*/;
    if(udpfd ==-1) exit(1);
    tcpfd= socket(AF_INET,SOCK_STREAM,0) /*TCP*/;
    if(tcpfd ==-1) printf("1");
    memset(&hints_fd,0,sizeof hints_fd);
    hints_fd.ai_family=AF_INET;
    hints_fd.ai_socktype=SOCK_DGRAM;
    hints_fd.ai_flags=AI_PASSIVE;
    
    memset(&hints_tcp,0,sizeof hints_tcp);
    hints_tcp.ai_family=AF_INET; //IPv4
    hints_tcp.ai_socktype=SOCK_STREAM; //TCP socket
    hints_tcp.ai_flags=AI_PASSIVE;

    sprintf(dsport, "%d",dsport_err);
    errcode=getaddrinfo(NULL,dsport,&hints_fd,&res);
    if(errcode != 0) printf("3");
    n= bind(udpfd,res->ai_addr,res->ai_addrlen);
    if(n==-1) printf("4");
    n= bind(tcpfd,res->ai_addr,res->ai_addrlen);
    if(n==-1) printf("Nao consegui dar bind tcpfd\n");
    puts("HELP2\n");
    if(listen(tcpfd,5) == -1) printf("7");
    puts("HELP3\n");    
    
    while(1){
        puts("HELP4\n");
        char command[13] = "";
        char buffer3[128] = "";
        FD_ZERO(&rfds);
        FD_SET(udpfd,&rfds);
        FD_SET(tcpfd,&rfds);maxfd=max(udpfd,tcpfd);
        puts("HELP5\n");
        
        puts("HELP6\n");
        checker=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
        puts("HELP7\n");
        if(checker <= 0) printf("6");
        puts("HELP8\n");
        if(FD_ISSET(udpfd,&rfds)){ /*UDP */
            switch(state){
                case idle:
                    printf("IDLE UDP\n");
                    state = busy;
                    addrlen = sizeof(addr);
                    n=recvfrom(udpfd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
                    if(n==-1) exit(1);
                    printf("isto e o buffer %s\n",buffer);
                    sscanf(buffer,"%s",command);
                    state = idle;
                    freeaddrinfo(res);
                case busy:
                    printf("IDLE BUSY UDP\n");
                    break;
            }
        }
        if(FD_ISSET(tcpfd,&rfds)){ /*TCP*/
            switch(state){
                case idle:
                    printf("IDLE TCP\n");
                    state = busy;
                    printf("BOM DIA1\n");
                    if(newfd = accept(tcpfd,(struct sockaddr*)&addr,&addrlen) == -1) printf("BOM DIA\n");
                    printf("BOM DIA2\n");
                    n = read(newfd,buffer3,1);
                    if (n== -1) printf("BOM DIA 4 \n");
                    printf("BOM DIA3\n");
                    write(1,"received: ",10);write(1,buffer,n);
                    n = write(newfd,buffer,n);
                    if( n == -1)exit(1);
                    close(newfd);
                    freeaddrinfo(res);
                case busy:
                    printf("IDLE BREAK TCP\n");
                    break;
            }
        }
        /*REG*/
        if(strcmp(command,"REG")== 0){
            
            char uid_str[6] = "";
            char password[9] = "";
            sscanf(buffer,"%s %s %s",command,uid_str,password);
            
            if(strlen(uid_str)!=5 || (strlen(password) !=8) || (n_clients >= 100000)){ /*ERROR*/
                 n = sendto(udpfd,"RRG NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                 if(n==-1) exit(1);
                 printf("Fiz isto1\n");
            }
            else if(SearchUID(uid_str) != 0){
                 n = sendto(udpfd,"RRG DUP\n",n,0,(struct sockaddr*)&addr,addrlen);
                 if(n==-1) exit(1);
                 printf("Fiz isto2\n");
            }
            else{
                if (CreateUserDir(uid_str,password) == 0){
                    n = sendto(udpfd,"RRG NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                else{
                    n = sendto(udpfd,"RRG OK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                    n_clients += 1;
                    printf("Fiz isto3\n");
                }  
            }   
        }
        else if(strcmp(command,"LOG") == 0){
            FILE *f;
            char uid_str[6] = "";
            char password[9] = "";
            char check_pass[9] = "";
            char user_login[30] = "";
            char user_password[30] = "";
            
            sscanf(buffer,"%s %s %s",command,uid_str,password);
            if(strlen(uid_str)!=5 || (strlen(password) !=8) || SearchUID(uid_str) == 0){ /*ERROR*/
                n = sendto(udpfd,"RLO NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                if(n==-1) exit(1);
            }
            else{    
                sprintf(user_password,"USERS/%s/%s_pass.txt",uid_str,uid_str);
                sprintf(user_login,"USERS/%s/%s_login.txt",uid_str,uid_str);
                f = fopen(user_password,"r");
                fscanf(f,"%8s",check_pass);
		        printf("ISTO È O CHECK PASS %s\n",check_pass);
                fclose(f);
            if (strcmp(check_pass, password )== 0){
                f = fopen(user_login,"w");
                if(f != NULL){
                    
                    n = sendto(udpfd,"RLO OK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                fclose(f);
            }
                else{
                    n = sendto(udpfd,"RLO NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
            }
        }
        else if(strcmp(command,"UNR") == 0){
            FILE *f;
            char uid_str[6] = "";
            char password[9] = "";
            char check_pass[9] = "";
            char user_login[30] = "";
            char user_password[30] = "";
            char group_member_path[30] = "";
            sscanf(buffer,"%s %s %s",command,uid_str,password);
            if(strlen(uid_str)!=5 || (strlen(password) !=8) || SearchUID(uid_str) == 0){ /*ERROR*/
                n = sendto(udpfd,"RUN NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                if(n==-1) exit(1);
            }
            else{
                sprintf(user_password,"USERS/%s/%s_pass.txt",uid_str,uid_str);
                sprintf(user_login,"USERS/%s/%s_login.txt",uid_str,uid_str);
                f = fopen(user_password,"r");
                fscanf(f,"%8s",check_pass);
		        printf("%s\n",check_pass);
                fclose(f);
                if(strcmp(password,check_pass) == 0){
                    GROUPLIST *list = malloc(sizeof(GROUPLIST));
                    ListGroupsDir(list);
                    for(int i=0; i < list->no_groups;i++){
                        sprintf(group_member_path,"GROUPS/%s/%s.txt",list->group_no[i],uid_str);
                        if(access( group_member_path, F_OK ) == 0){
                            unlink(group_member_path);
                        }
                    }
                    free(list);
                    DelPassFile(uid_str);
                    DelLoginFile(uid_str);
                    DelUserDir(uid_str);
                    n = sendto(udpfd,"RUN OK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                else{
                    n = sendto(udpfd,"RUN NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
            }

        }
        else if(strcmp(command,"OUT") == 0){
            FILE *f;
            char uid_str[6] = "";
            char password[9] = "";
            char check_pass[9] = "";
            char user_login[30] = "";
            char user_password[30] = "";
            
            sscanf(buffer,"%s %s %s",command,uid_str,password);
            if(strlen(uid_str)!=5 || (strlen(password) !=8) || SearchUID(uid_str) == 0){ /*ERROR*/
                n = sendto(udpfd,"ROU NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                if(n==-1) exit(1);
            }
            else{
                sprintf(user_password,"USERS/%s/%s_pass.txt",uid_str,uid_str);
                sprintf(user_login,"USERS/%s/%s_login.txt",uid_str,uid_str);
                f = fopen(user_password,"r");
                fscanf(f,"%8s",check_pass);
                fclose(f);
                f = fopen(user_login,"r");
                if(strcmp(password,check_pass) == 0 && f != NULL){
                    fclose(f);
                    DelLoginFile(uid_str);
                    n = sendto(udpfd,"ROU OK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                else{
                    n = sendto(udpfd,"ROU NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
            }

        }

        else if(strcmp(command,"GLS") == 0){
            char send[3070] = "";
            char auxiliar[50] = "";
            GROUPLIST *list = malloc(sizeof(GROUPLIST));
            ListGroupsDir(list);
            sprintf(send,"RGL %d",list->no_groups);
            for(int i=0; i < list->no_groups;i++){
                sprintf(auxiliar," %s %s %s",list->group_no[i],list->group_name[i],list->group_message[i]);
                strcat(send,auxiliar);
            }
            strcat(send,"\n");
            free(list);
            n = sendto(udpfd,send,strlen(send) ,0,(struct sockaddr*)&addr,addrlen);
            if(n==-1) exit(1);
            
        }
        else if(strcmp(command,"GLM") == 0){
            char send[3070] = "";
            char aux_big[3070] = "";
            char auxiliar[50] = "";
            char uid_str[6] = "";
            char user_login[30] = "";
            char group_member_path[30] = "";
            int counter = 0;
            sscanf(buffer,"%s %s",command,uid_str);
            sprintf(user_login,"USERS/%s/%s_login.txt",uid_str,uid_str);
                
            if(strlen(uid_str)!=5  || access( user_login, F_OK ) != 0){
                    n = sendto(udpfd,"RGM E_USR\n",11,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
            }
            else{
                GROUPLIST *list = malloc(sizeof(GROUPLIST));
                ListGroupsDir(list);
                for(int i=0; i < list->no_groups;i++){
                    sprintf(group_member_path,"GROUPS/%s/%s.txt",list->group_no[i],uid_str);
                    if(access( group_member_path, F_OK ) == 0){
                        sprintf(auxiliar," %s %s %s",list->group_no[i],list->group_name[i],list->group_message[i]);
                        strcat(aux_big,auxiliar);
                        counter++;
                    }
                }
                
                if(counter == 0){
                    free(list);
                    n = sendto(udpfd,"RGM 0\n",7,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                else{
                    sprintf(auxiliar,"RGM %d",counter);
                    strcat(send,auxiliar);
                    strcat(send,aux_big);
                    strcat(send,"\n");
                    free(list);       
                    n = sendto(udpfd,send,strlen(send) ,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
            }
        }


       
       
        else if(strcmp(command,"GSR") == 0){
            
                FILE *f;
                char uid_str[6] = "";
                char gid_str[3] = "";
                char gname[25] =  "";
                char gname_checker[25] = "";
                char user_login[30] = "";
                char group_gid_dir[10] = "";
                char group_namef[32] = "";
                
        
                sscanf(buffer,"%s %s %s %s",command,uid_str,gid_str,gname);
                
                sprintf(user_login,"USERS/%s/%s_login.txt",uid_str,uid_str);
                
                if(strlen(uid_str)!=5  || strlen(gname) > 25|| n_groups >= 99){
                    n = sendto(udpfd,"RGS NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                    
                }
                
                if( access( user_login, F_OK ) != 0 ){ /*NOT LOGGED IN*/
                    
                    n = sendto(udpfd,"RGS E_USR\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                
                else if(strcmp(gid_str,"00") != 0){ /*NO CREATION*/
                    
                    sprintf(group_gid_dir,"GROUPS/%s",gid_str);
                    DIR *d;
                    d = opendir(group_gid_dir);
                    if(d ==NULL){
                        n = sendto(udpfd,"RGS E_GRP\n",n,0,(struct sockaddr*)&addr,addrlen); 
                        if(n==-1) exit(1);
                    }
                    else{
                        
                        sprintf(group_namef,"%s/%s_name.txt",group_gid_dir,gid_str);
                        f = fopen(group_namef,"r");
                        fscanf(f,"%24s",gname_checker);
                        fclose(f);
                        if(strcmp(gname_checker,gname)!= 0){
                            n = sendto(udpfd,"RGS E_GNAME\n",n,0,(struct sockaddr*)&addr,addrlen); 
                            if(n==-1) exit(1);
                        }
                        else{
                            char subscribe[21] = "";
                            sprintf(subscribe,"%s/%s.txt",group_gid_dir,uid_str);
                            f = fopen(subscribe,"w");
                            fclose(f);
                            n = sendto(udpfd,"RGS OK\n",n,0,(struct sockaddr*)&addr,addrlen); 
                            if(n==-1) exit(1);
                        }
                    }
                    closedir(d);
                }
                else if(strcmp(gid_str,"00") == 0){
                    char send[20] = "";
                    char n_groups_str[3] = "";
                    int ret;
                   
                    int name_exists = -1;
                    GROUPLIST *list = malloc(sizeof(GROUPLIST));
                    ListGroupsDir(list);
                     for(int i=0; i < list->no_groups;i++){
                        if(strcmp(gname,list->group_name[i]) == 0){
                            name_exists = 1;
                        }
                    }
                    free(list);
                    if(name_exists == 1){
                        n = sendto(udpfd,"RGS E_GNAME\n",n,0,(struct sockaddr*)&addr,addrlen); 
                        if(n==-1) exit(1);
                    }
                    else{
                        n_groups++;
                        if(n_groups < 10){
                            sprintf(n_groups_str,"0%d",n_groups);
                            }
                        else if(n_groups >= 10){
                            sprintf(n_groups_str,"%d",n_groups);
                        }
                        sprintf(group_gid_dir,"GROUPS/%s",n_groups_str);
                        ret=mkdir(group_gid_dir,0700);
                        sprintf(group_namef,"%s/%s_name.txt",group_gid_dir,n_groups_str);
                        f = fopen(group_namef,"w");
                        fputs(gname,f);
                        fclose(f);
                        char subscribe[21] = "";
                        sprintf(subscribe,"%s/%s.txt",group_gid_dir,uid_str);
                        f = fopen(subscribe,"w");
                        fclose(f);
                        sprintf(send,"RGS NEW %s\n",n_groups_str);
                        n = sendto(udpfd,send,n,0,(struct sockaddr*)&addr,addrlen); 
                        if(n==-1) exit(1);
                    }
                }
                
                    
        }
        else if(strcmp(command,"GUR") == 0){
                FILE *f;
                
                char uid_str[6] = "";
                char gid_str[3] = "";
                char user_login[30] = "";
                char group_gid_name[25] = "";
                char dir_group_sub[25] = "";
                char group_namef[32] = "";
                int error;
                
        
                sscanf(buffer,"%s %s %s",command,uid_str,gid_str);
                
                sprintf(user_login,"USERS/%s/%s_login.txt",uid_str,uid_str);
                sprintf(group_gid_name,"GROUPS/%s/%s_name.txt",gid_str,gid_str);
                sprintf(dir_group_sub,"GROUPS/%s/%s.txt",gid_str,uid_str);
                if(strlen(uid_str)!=5  ||strlen(gid_str) !=2){
                    n = sendto(udpfd,"RGU NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                
                else if( access( user_login, F_OK ) != 0 ){ /*NOT LOGGED IN*/
                    n = sendto(udpfd,"RGU E_USR\n",n,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) exit(1);
                }
                else if(access( group_gid_name, F_OK ) != 0){ /* GROUP Doesnt Exist */
                    n = sendto(udpfd,"RGU E_GRP\n",n,0,(struct sockaddr*)&addr,addrlen); 
                    if(n==-1) exit(1);
                }
                 else if(access( dir_group_sub, F_OK ) != 0){ /* User is not subbed*/
                    n = sendto(udpfd,"RGU E_USR\n",n,0,(struct sockaddr*)&addr,addrlen); 
                    if(n==-1) exit(1);
                }
                else{
                    error = unlink(dir_group_sub);
                    if(error == 0){
                        n = sendto(udpfd,"RGU OK\n",n,0,(struct sockaddr*)&addr,addrlen); 
                        if(n==-1) exit(1);
                    }
                    else{
                        n = sendto(udpfd,"RGU NOK\n",n,0,(struct sockaddr*)&addr,addrlen);
                        if(n==-1) exit(1);
                    }
                }
        }
        /*END OF ALL UDP COMMANDS */




































    }/*END OF WHILE*/
    
    /*endof*/
    freeaddrinfo(res);
    close(udpfd);

    printf("DSPORT %d Flag %d\n",dsport_err,flag_v);
    
    return 0;
    
}