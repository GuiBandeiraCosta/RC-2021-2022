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
char user_logged[6] = "";
char logged_pass[9] = "";
char gid_selected[3] = "";

/* Connection*/
int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
/*end*/

int TimerON(int sd){
    struct timeval tmout;
    memset((char *)&tmout,0,sizeof(tmout)); /* clear time structure */
    tmout.tv_sec=5; /* Wait for 15 sec for a reply from server. */
    return (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tmout,sizeof(struct timeval)));
}

int TimerOFF(int sd){
    struct timeval tmout;
    memset((char *)&tmout,0,sizeof(tmout)); /* clear time structure */
    return (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tmout,sizeof(struct timeval)));
}



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
        char input[240];
        char command[13];
        fgets(input,240,stdin);
        sscanf(input,"%s",command);
        if(strcmp(command,"reg")== 0){
            char send[20];
            char uid_str[6];
            char password[9];
            
            sscanf(input,"%s %s %s",command,uid_str,password);
            
            
            if(strlen(uid_str)!=5){ 
                printf("Invalid UID: Must be 5 digits long\n");
               
            }
            
            else if(strlen(password) !=8){
                printf("Password must be 8 digits long\n");
               
            }
            else{
                char buffer[10];
                sprintf(send,"REG %s %s\n",uid_str,password);
                n=sendto(fd,send,strlen(send),0,res->ai_addr,res->ai_addrlen);
                if(n==-1)  exit(1);
                TimerON(fd);
                addrlen=sizeof(addr);
                n=recvfrom(fd,buffer,10,0,(struct sockaddr*)&addr,&addrlen);
                if(n==-1)  printf("Server error try again please\n");
                else{
                TimerOFF(fd);
            
                buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                if(strcmp(buffer,"RRG OK") == 0){ 
                    printf("User successfully registered\n");
                }
                else if(strcmp(buffer,"RRG DUP") == 0){
                    printf("User already registered\n");
                }
                else if(strcmp(buffer,"RRG NOK") == 0){
                    printf("Registration failed");
                }
                else if(strcmp(buffer,"ERR") == 0){
                    printf("ERROR:unexpected protocol message\n");
                }
                else{
                    printf("Something went wrong,try again\n");
                }
            }      
        }
        }
        else if(strcmp(command,"login") == 0){
            char send[20];
            char uid_str[6];
            char password[9];
            sscanf(input,"%s %s %s",command,uid_str,password);
            if((strcmp(user_logged,uid_str) != 0 || strcmp(password,logged_pass) != 0 )&&(strcmp(user_logged,"") != 0 && strcmp(logged_pass,"") != 0)){
                        printf("There is a User already logged in\n");
            }
            
            
            else if(strlen(uid_str)!=5){ 
                printf("Invalid UID: Must be 5 digits long\n");     
            }
            else if(strlen(password) !=8){
                printf("Password must be 8 digits long\n");   
            }
            else{
                char buffer[10];
                sprintf(send,"LOG %s %s\n",uid_str,password);
                n=sendto(fd,send,strlen(send),0,res->ai_addr,res->ai_addrlen);
                if(n==-1)  exit(1);
                TimerON(fd);
                addrlen=sizeof(addr);
                n=recvfrom(fd,buffer,10,0,(struct sockaddr*)&addr,&addrlen);
                
                if(n==-1)  printf("Server error try again please\n");
                else{
                    buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                    TimerOFF(fd);
                    if(strcmp(buffer,"RLO OK") == 0){
                        strcpy(user_logged,uid_str);
                        strcpy(logged_pass,password);
                        printf("You are now logged in\n");
                    }
                    else if(strcmp(buffer,"RLO NOK") == 0){
                        printf("Couldnt login\n");
                    }
                    else{
                        printf("Something went wrong,try again\n");
                    }  
            }  
            }
        }
        else if(strcmp(command,"unregister")==0 || strcmp(command,"unr")==0){
            char send[20];
            char uid_str[6];
            char password[9];
            sscanf(input,"%s %s %s",command,uid_str,password);
            if(strlen(uid_str)!=5){ 
                printf("Invalid UID: Must be 5 digits long\n");     
            }
            else if(strlen(password) !=8){
                printf("Password must be 8 digits long\n");   
            }
            else{
                char buffer[10];
                sprintf(send,"UNR %s %s\n",uid_str,password);
                n=sendto(fd,send,strlen(send),0,res->ai_addr,res->ai_addrlen);
                if(n==-1)  exit(1);
                TimerON(fd);
                addrlen=sizeof(addr);
                n=recvfrom(fd,buffer,10,0,(struct sockaddr*)&addr,&addrlen);
                if(n==-1)  printf("Server error try again please\n");
                else{
                    TimerOFF(fd);
                    buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                    if(strcmp(buffer,"RUN OK") == 0){
                        printf("User successfuly unregistered\n");
                    }
                    else if(strcmp(buffer,"RUN NOK") == 0){
                        printf("Couldnt unregister\n");
                    }
                    else{
                    printf("Something went wrong,try again\n");
                    } 
                }
                
            }
            
        }
        else if(strcmp(command,"logout") == 0){
            char send[20];
            if(strcmp(user_logged,"") == 0 || strcmp(logged_pass,"") == 0){
                printf("User not logged in\n");
            }
            else{
                char buffer[10];
                sprintf(send,"OUT %s %s\n",user_logged,logged_pass);
                n = sendto(fd,send,strlen(send),0,res->ai_addr,res->ai_addrlen);
                if(n == -1) exit(1);
                TimerON(fd);
                addrlen = sizeof(addr);
                n=recvfrom(fd,buffer,10,0,(struct sockaddr*)&addr,&addrlen);
                if(n==-1)  printf("Server error try again please\n");
                else{
                    TimerOFF(fd);
                    buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                    if(strcmp(buffer,"ROU OK") == 0){
                        strcpy(user_logged,"");
                        strcpy(logged_pass,"");
                        printf("User successfuly logged out\n");
                    }
                    else if(strcmp(buffer,"ROU NOK") == 0){
                        printf("Couldnt logout\n");
                    }
                    else{
                    printf("Something went wrong,try again\n");
                    }     
                }
            }
        }
        else if(strcmp(command,"showuid")==0 || strcmp(command,"su")==0){
            if(strcmp(user_logged,"") == 0){
                printf("You are not logged in!\n"); 
            }
            else{
            printf("USER LOGGED IN ID %s\n",user_logged);
            }
        }
        
        
        else if(strcmp(command,"groups")==0 || strcmp(command,"gl")==0){
            char buffer[3070]; /*MAX AMOUNT*/
            int N_Groups;
            char previous[3070]; 
            char *list; 
            char auxiliar[30];
            
            n = sendto(fd,"GLS\n",4,0,res->ai_addr,res->ai_addrlen);
            if(n == -1) exit(1); 
            addrlen = sizeof(addr);
            TimerON(fd);
            n=recvfrom(fd,buffer,3070,0,(struct sockaddr*)&addr,&addrlen);
            if(n==-1)  printf("Server error try again please\n");
            
            else{
                buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                TimerOFF(fd);
                if(strcmp(buffer,"RGL 0") == 0){
                    printf("There are no Groups\n");
                }
                else{
                
                    list= strtok(buffer," ");  
                    list= strtok(NULL," ");  
                    list= strtok(NULL," "); 
                    strcpy(previous, ""); 
                    while (list != NULL){
                        sprintf(auxiliar,"Group %s:",list);
                        strcat(previous,auxiliar);
                        list= strtok(NULL," "); 
                        sprintf(auxiliar," %s ",list);
                        strcat(previous,auxiliar);
                        list= strtok(NULL," "); 
                        sprintf(auxiliar,"Last MSG: %s\n",list);
                        strcat(previous,auxiliar);
                        list= strtok(NULL," ");
                    }
                    printf("%s", previous); 
                }
            }
        }

        else if(strcmp(command,"my_groups")==0 || strcmp(command,"mgl")==0){
            if(strcmp(user_logged,"") == 0){
                printf("You are not logged in!\n"); 
            }
            else{
                char buffer[3070]; /*MAX AMOUNT*/
                int N_Groups;
                char send[12];
                char previous[3070];
                char *list; 
                char auxiliar[30];
                sprintf(send,"GLM %s",user_logged);
                n = sendto(fd,send,strlen(send),0,res->ai_addr,res->ai_addrlen);
                if(n == -1) exit(1); 
                addrlen = sizeof(addr);
                TimerON(fd);
                n=recvfrom(fd,buffer,3070,0,(struct sockaddr*)&addr,&addrlen);
                
                if(n==-1)  printf("Server error try again please\n");
            
                else{
                    buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                    TimerOFF(fd);
                    if(strcmp(buffer,"RGM E_USR") == 0){
                        printf("Invalid User ID\n");
                    }
                    else if(strcmp(buffer,"RGM 0") == 0){
                        printf("User isnt subscribed to any groups\n");
                    }
                    else{
                    
                        list= strtok(buffer," ");  
                        list= strtok(NULL," ");  
                        list= strtok(NULL," "); 
                        strcpy(previous, ""); 
                        while (list != NULL){
                            sprintf(auxiliar,"Group %s:",list);
                            strcat(previous,auxiliar);
                            list= strtok(NULL," "); 
                            sprintf(auxiliar," %s ",list);
                            strcat(previous,auxiliar);
                            list= strtok(NULL," "); 
                            sprintf(auxiliar,"Last MSG: %s\n",list);
                            strcat(previous,auxiliar);
                            list= strtok(NULL," ");
                        }
                        printf("%s", previous); 
                    }
                }   
            }
        }

        else if(strcmp(command,"select")==0 || strcmp(command,"sag")==0){
            int gid;
            sscanf(input,"%s %d",command,&gid);
            if(strcmp(user_logged,"") == 0){
                printf("You are not logged in!\n"); 
            }
            else if(gid < 0 || gid > 99){
                printf("GID must be between 0 and 99\n");
            }
            
            else{
                if(gid < 10){
                    sprintf(gid_selected,"0%d",gid);
                }
                else if(gid >= 10){
                    sprintf(gid_selected,"%d",gid);
                }
                printf("Selected GID %s\n",gid_selected);
            }
        }

        else if(strcmp(command,"showgid")==0 || strcmp(command,"sg")==0){
            if(strcmp(user_logged,"") == 0){
                printf("You are not logged in!\n"); 
            }
            else if(strcmp(gid_selected,"") == 0){
                printf("No group selected yet\n"); 
            }
            else{
            printf("GROUP ID %s\n",gid_selected);
            }
        }

        else if(strcmp(command,"subscribe")==0 || strcmp(command,"s")==0){
            char send[50];
            char gname[25];
            int gid;
            char gid_str[3];
            sscanf(input,"%s %d %s",command,&gid,gname);
            if(strcmp(user_logged,"") == 0){ /*Check User logged in*/
                printf("User must be logged in\n");
            }
            
            else if(gid < 0 || gid > 99){
                printf("GID must be between 0 and 99");
            }
            else if(strlen(gname) > 24){ /*Check correct lenght group name */
                printf("Gname cannot exceed 24 charachters\n");
            }
            
            
            else{
                char buffer[15];
                if(gid < 10){
                    sprintf(gid_str,"0%d",gid);
                }
                else if(gid >= 10){
                    sprintf(gid_str,"%d",gid);
                }
                
                sprintf(send,"GSR %s %s %s\n",user_logged,gid_str,gname);
                n = sendto(fd,send,strlen(send),0,res->ai_addr,res->ai_addrlen);
                if(n == -1) exit(1);
                
                addrlen = sizeof(addr);
                TimerON(fd);
                n=recvfrom(fd,buffer,15,0,(struct sockaddr*)&addr,&addrlen);
                if(n==-1)  printf("Server error try again please\n");
                else{
                    char RGS[4];
                    char RGS_STATUS[8];
                    char gid_created[3];
                    buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                    sscanf(buffer,"%s %s %s",RGS,RGS_STATUS,gid_created);
                    TimerOFF(fd);
                    if(strcmp(buffer,"RGS OK") == 0){
                        printf("Group subscribed to\n");
                    }
                    else if(strcmp(RGS,"RGS") == 0 && strcmp(RGS_STATUS,"NEW") == 0){
                        printf("User %s created group %s named %s\nUser %s subscribed to group %s\n",user_logged,gid_created,gname,user_logged,gid_created);
                        
                    }
                    else if(strcmp(buffer,"RGS E_USR") == 0){
                        printf("Invalid user\n");
                    }
                    else if(strcmp(buffer,"RGS E_GRP") == 0){
                        printf("Invalid group number\n");
                    }
                    else if(strcmp(buffer,"RGS E_GNAME") == 0){
                        printf("Invalid group name\n");
                    }
                    else if(strcmp(buffer,"RGS E_FULL") == 0){
                        printf("Maximiun group number reched\n");
                    }
                    else if(strcmp(buffer,"RGS NOK") == 0){
                        printf("STATUS NOT OKAY\n");
                    }
                    else{
                    printf("Something went wrong,try again\n");
                    }
                }
            }
        }

        else if(strcmp(command,"unsubscribe")==0 || strcmp(command,"u")==0){
            char send[50];
            char gname[25];
            int gid;
            char gid_str[3];
            sscanf(input,"%s %d",command,&gid);
            if(strcmp(user_logged,"") == 0){ /*Check User logged in*/
                printf("User must be logged in\n");
            }
            
            else if(gid <= 0 || gid > 99){
                printf("GID must be between 1 and 99");
            }
            else{
                char buffer[12];
                if(gid < 10){
                    sprintf(gid_str,"0%d",gid);
                }
                else if(gid >= 10){
                    sprintf(gid_str,"%d",gid);
                }
                
                sprintf(send,"GUR %s %s\n",user_logged,gid_str);
                n = sendto(fd,send,strlen(send),0,res->ai_addr,res->ai_addrlen);
                if(n == -1) exit(1);
                addrlen = sizeof(addr);
                TimerON(fd);
                n=recvfrom(fd,buffer,12,0,(struct sockaddr*)&addr,&addrlen);
                if(n==-1)  printf("Server error try again please\n");
                else{
                    TimerOFF(fd);
                    buffer[strcspn(buffer, "\n")] = 0; /*Removes \n from buffer */
                    if(strcmp(buffer,"RGU OK") == 0){
                        printf("User %s unsubscribed to GROUP %s\n",user_logged,gid_str);
                    }
                    else if(strcmp(buffer,"RGU E_USR") == 0){
                        printf("Invalid User ID\n");
                    }
                    else if(strcmp(buffer,"RGU E_GRP") == 0){
                        printf("Invalid Group ID\n");
                    }
                    else if(strcmp(buffer,"RGU NOK") == 0){
                        printf("Couldnt unsubscribe\n");
                    }
                    else{
                        printf("Something went wrong,try again\n");
                    }
                }   
            }
        }   

        
    }/*END OF WHILE*/
    

    freeaddrinfo(res);
    close(fd);

   
    return 0 ;
}
    
