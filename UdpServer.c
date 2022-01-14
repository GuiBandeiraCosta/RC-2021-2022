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
struct addrinfo hints_fd,hints_tcp,*res_udp,*res_tcp;
struct sockaddr_in addr;

/*end*/
int UsersinGroup(char buffer[],char gid[]){
    DIR *d;
    struct dirent *dir;
    FILE *fp;
    char dirpath[11];
    char namepath[25];
    char group_name[25];
    char namefile[13];
    sprintf(dirpath,"GROUPS/%s",gid);
    
    d = opendir(dirpath);
    printf("Dir path %s\n",dirpath);
    if(d == NULL){
        strcpy(buffer,"RUL NOK\n");
        return 0;
    }
    
    else{
        sprintf(namefile,"%s_name.txt",gid);
        sprintf(namepath,"%s/%s",dirpath,namefile);
        printf("name %s\n",namepath);
        
        fp = fopen(namepath,"r");
        fscanf(fp,"%24s",group_name);
        fclose(fp);
        sprintf(buffer,"RUL OK %s",group_name);
        while ((dir = readdir(d)) != NULL){
            if(strlen(dir->d_name)>4 && strcmp(dir->d_name,namefile) != 0){ /*Garanties that only uid.txt files are stored in Buffer*/
                    char name[6];
                    strcat(buffer," ");
                    sprintf(name,"%.5s",dir->d_name);
                    strcat(buffer,name);    
            }
        }
        strcat(buffer,"\n");
        printf("BUFFER %s",buffer);
        
        closedir(d);
    }
    return 0;
    
}

char *remove_white_spaces(char *str)
{
	int i = 0, j = 0;
	while (str[i])
	{
		if (str[i] != ' ')
          str[j++] = str[i];
		i++;
	}
	str[j] = '\0';
	return str;
}

int Messages_in_group(char gid[],char nmsg[],int flag){
    DIR *d;
    struct dirent *dir;
    int i=0;
    char msg_dir[30] = "";
    sprintf(msg_dir,"GROUPS/%s/MSG",gid);
    d = opendir(msg_dir);
    if (d){
        while ((dir = readdir(d)) != NULL){
            if(dir->d_name[0]=='.')
                continue;
            else{ i++;}
        }
        if (flag == 1){
            i++;
        }
        if(i < 10){ sprintf(nmsg,"000%d",i);}
        else if(10 <= i < 100){ sprintf(nmsg,"00%d",i);}
        else if(100 <= i < 1000){ sprintf(nmsg,"0%d",i);}
        else{sprintf(nmsg,"0%d",i);}
    }
}

int ListGroupsDir(GROUPLIST *list){
    DIR *d, *d_msg;
    struct dirent *dir,*dir_msg;
    char msg_dir[20] = "";
    int i=0;
    FILE *fp;
    char GIDname[30] = "";
    list->no_groups=0;
    d = opendir("GROUPS");
    if (d){
        while ((dir = readdir(d)) != NULL){
            char last_msg[6] = "";
            if(dir->d_name[0]=='.')
                continue;
            if(strlen(dir->d_name)>2)
                continue;
            sprintf(msg_dir,"GROUPS/%.2s/MSG",dir->d_name);
            
            d_msg = opendir(msg_dir);
            while ((dir_msg = readdir(d_msg)) != NULL){
                
                if(dir_msg->d_name[0]=='.'){
                    continue;
                }
                sprintf(last_msg,"%.4s",dir_msg->d_name);
                strcpy(list->group_message[i],last_msg);
            }
            closedir(d_msg);
            if(strcmp(last_msg,"") == 0){
                strcpy(list->group_message[i],"0000");
            }
            strcpy(list->group_no[i], dir->d_name);
            sprintf(GIDname,"GROUPS/%.2s/%.2s_name.txt",dir->d_name,dir->d_name);
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

int getFile(char gid[], char mid[], char filename[]){
    DIR *d;
    struct dirent *dir;
    
    char msg_dir[30] = "";
    sprintf(msg_dir,"GROUPS/%s/MSG/%s",gid, mid);
    d = opendir(msg_dir);
    if (d){
        while ((dir = readdir(d)) != NULL){
            if(dir->d_name[0]=='.'){
                continue;
            }
            if(strcmp(dir->d_name, "A U T H O R.txt")!=0 && strcmp(dir->d_name, "T E X T.txt")!=0 ){
                strcpy(filename,dir->d_name);
                 
            }
        }
    }
    
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

int UdpCommands(char buffer[]){
    char command[13] = "";
    sscanf(buffer,"%s",command);
    /*REG*/
        if(strcmp(command,"REG")== 0){
            
            char uid_str[6] = "";
            char password[9] = "";
            sscanf(buffer,"%s %s %s",command,uid_str,password);
            
            if(strlen(uid_str)!=5 || (strlen(password) !=8) || (n_clients >= 100000)){ /*ERROR*/
                 n = sendto(udpfd,"RRG NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                 if(n==-1) printf("Send to Failed on Reg\n");
            }
            else if(SearchUID(uid_str) != 0){
                 n = sendto(udpfd,"RRG DUP\n",8,0,(struct sockaddr*)&addr,addrlen);
                 if(n==-1) printf("Send to Failed on Reg\n");
                 if(flag_v==1){
                        printf("UID=%s: User Duplicate\n", uid_str); 
                }
            }
            else{
                if (CreateUserDir(uid_str,password) == 0){
                    n = sendto(udpfd,"RRG NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on Reg\n");
                }
                else{
                    n = sendto(udpfd,"RRG OK\n",7,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on Reg\n");
                    n_clients += 1;
                    if(flag_v==1){
                        printf("UID=%s: new user\n", uid_str); 
                    }
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
                n = sendto(udpfd,"RLO NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                if(n==-1) printf("Send to Failed on LOG\n");
            }
            else{    
                sprintf(user_password,"USERS/%s/%s_pass.txt",uid_str,uid_str);
                sprintf(user_login,"USERS/%s/%s_login.txt",uid_str,uid_str);
                f = fopen(user_password,"r");
                fscanf(f,"%8s",check_pass);
                fclose(f);
            if (strcmp(check_pass, password )== 0){
                f = fopen(user_login,"w");
                if(f != NULL){
                    
                    n = sendto(udpfd,"RLO OK\n",7,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on LOG\n");
                     if(flag_v==1){
                        printf("UID=%s: login ok\n", uid_str); 
                    }
                }
                fclose(f);
            }
                else{
                    n = sendto(udpfd,"RLO NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on LOG\n");
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
                n = sendto(udpfd,"RUN NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                if(n==-1) printf("Send to Failed on UNR\n");
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
                    n = sendto(udpfd,"RUN OK\n",7,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on UNR\n");
                    if(flag_v==1){
                        printf("UID=%s: User Unregister\n", uid_str); 
                    }
                }
                else{
                    n = sendto(udpfd,"RUN NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on UNR\n");
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
                n = sendto(udpfd,"ROU NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                if(n==-1) printf("Send to Failed on OUT\n");
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
                    n = sendto(udpfd,"ROU OK\n",7,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on OUT\n");
                    if(flag_v==1){
                        printf("UID=%s: User logout\n", uid_str); 
                    }
                }
                else{
                    n = sendto(udpfd,"ROU NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on OUT\n");
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
            if(n==-1) printf("Send to Failed on GLS\n");
            else if(flag_v==1){
                    printf("Command Groups emmited\n");
            }
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
                    n = sendto(udpfd,"RGM E_USR\n",10,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on OUT\n");
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
                    n = sendto(udpfd,"RGM 0\n",6,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on GLM\n");
                    if(flag_v==1){
                        printf("UID=%s: My Groups Empty\n", uid_str); 
                    }
                }
                else{
                    sprintf(auxiliar,"RGM %d",counter);
                    strcat(send,auxiliar);
                    strcat(send,aux_big);
                    strcat(send,"\n");
                    free(list);       
                    n = sendto(udpfd,send,strlen(send) ,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on GLM\n");
                     if(flag_v==1){
                        printf("UID=%s: My Groups Ok\n", uid_str); 
                    }
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
                    n = sendto(udpfd,"RGS NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on GSR\n");
                    
                }
                
                if( access( user_login, F_OK ) != 0 ){ /*NOT LOGGED IN*/
                    
                    n = sendto(udpfd,"RGS E_USR\n",10,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on GSR\n");
                }
                
                else if(strcmp(gid_str,"00") != 0){ /*NO CREATION*/
                    
                    sprintf(group_gid_dir,"GROUPS/%s",gid_str);
                    DIR *d;
                    d = opendir(group_gid_dir);
                    if(d ==NULL){
                        n = sendto(udpfd,"RGS E_GRP\n",10,0,(struct sockaddr*)&addr,addrlen); 
                        if(n==-1) printf("Send to Failed on GSR\n");
                    }
                    else{
                        
                        sprintf(group_namef,"%s/%s_name.txt",group_gid_dir,gid_str);
                        f = fopen(group_namef,"r");
                        fscanf(f,"%24s",gname_checker);
                        fclose(f);
                        if(strcmp(gname_checker,gname)!= 0){
                            n = sendto(udpfd,"RGS E_GNAME\n",12,0,(struct sockaddr*)&addr,addrlen); 
                            if(n==-1) printf("Send to Failed on GSR\n");
                        }
                        else{
                            char subscribe[21] = "";
                            sprintf(subscribe,"%s/%s.txt",group_gid_dir,uid_str);
                            f = fopen(subscribe,"w");
                            fclose(f);
                            n = sendto(udpfd,"RGS OK\n",7,0,(struct sockaddr*)&addr,addrlen); 
                            if(n==-1) printf("Send to Failed on GSR\n");
                            if(flag_v==1){
                            printf("UID=%s: User subscribed: %s - \"%s\" \n",uid_str,gid_str,gname); 
                            }         
                        }
                    }
                    closedir(d);
                }
                else if(strcmp(gid_str,"00") == 0){
                    char send[20] = "";
                    char n_groups_str[3] = "";
                    int ret;
                    char msg_dir[16] = " ";
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
                        n = sendto(udpfd,"RGS E_GNAME\n",12,0,(struct sockaddr*)&addr,addrlen); 
                        if(n==-1) printf("Send to Failed on GSR\n");
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
                        sprintf(msg_dir,"%s/MSG",group_gid_dir);                         
                        ret = mkdir(msg_dir,0700);
                        sprintf(group_namef,"%s/%s_name.txt",group_gid_dir,n_groups_str);
                        f = fopen(group_namef,"w");
                        fputs(gname,f);
                        fclose(f);
                        char subscribe[21] = "";
                        sprintf(subscribe,"%s/%s.txt",group_gid_dir,uid_str);
                        f = fopen(subscribe,"w");
                        fclose(f);
                        sprintf(send,"RGS NEW %s\n",n_groups_str);
                        n = sendto(udpfd,send,strlen(send),0,(struct sockaddr*)&addr,addrlen); 
                        if(n==-1) printf("Send to Failed on GSR\n");
                        if(flag_v==1){
                            printf("UID=%s: new group: %s - \"%s\" \n",uid_str,n_groups_str,gname); 
                        }   
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
                    n = sendto(udpfd,"RGU NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on GUR\n");
                }
                
                else if( access( user_login, F_OK ) != 0 ){ /*NOT LOGGED IN*/
                    n = sendto(udpfd,"RGU E_USR\n",10,0,(struct sockaddr*)&addr,addrlen);
                    if(n==-1) printf("Send to Failed on GUR\n");
                }
                else if(access( group_gid_name, F_OK ) != 0){ /* GROUP Doesnt Exist */
                    n = sendto(udpfd,"RGU E_GRP\n",10,0,(struct sockaddr*)&addr,addrlen); 
                    if(n==-1) printf("Send to Failed on GUR\n");
                }
                 else if(access( dir_group_sub, F_OK ) != 0){ /* User is not subbed*/
                    n = sendto(udpfd,"RGU E_USR\n",n,10,(struct sockaddr*)&addr,addrlen); 
                    if(n==-1) printf("Send to Failed on GUR\n");
                }
                else{
                    error = unlink(dir_group_sub);
                    if(error == 0){
                        n = sendto(udpfd,"RGU OK\n",7,0,(struct sockaddr*)&addr,addrlen); 
                        if(flag_v==1){
                            printf("UID=%.5s: User Unsubscribed: %.2s\n",uid_str,gid_str); 
                        } 
                        if(n==-1) printf("Send to Failed on GUR\n");
                    }
                    else{
                        n = sendto(udpfd,"RGU NOK\n",8,0,(struct sockaddr*)&addr,addrlen);
                        if(n==-1) printf("Send to Failed on GUR\n");
                    }
                }
        }
        return 0;
    /*END OF ALL UDP COMMANDS */
}

int ulist(){
    char buffer[630] = "";
    char gid[4] = "";
    char *ptr;
    ssize_t nbytes,nleft,nwritten,nread;
    ptr = gid;
    nleft= 2;
    while(nleft > 0){
        nread=read(newfd,ptr,1);
        if(nread==-1)/*error*/{printf("Read Failed on ULIST\n");return 0;}
        else if(nread==0)break;//closed by peer
        nleft-=nread;
        ptr+=nread;
    }
    UsersinGroup(buffer,gid);
    nleft = strlen(buffer);
    
    ptr = buffer;
    while(nleft > 0){
        nread=write(newfd,ptr,nleft);
        
        if(nread==-1)/*error*/{printf("Read Failed on ULIST\n");return 0;}
        else if(nread==0)break;//closed by peer
        nleft-=nread;
        ptr+=nread;
    }
    close(newfd);
    if(flag_v == 1){
        printf("Command Ulist called for group %.2s\n",gid);
    }
}

int post(){
    char *ptr;
    ssize_t nbytes,nleft,nwritten,nread;
    char uid[7]="";
    char gid[4]="";
    char tsize[12]="";
    char text[242]="";
    char spaces[3]="";
    char filename[27]="";
    char msg_dir[30] ="";
    char fsize[12] = "";
    char n_msg[5] = "";
    int flag = 0;
    char file_path[60]="";
    
    ptr = uid;
    nleft = 6;
    while(nleft>0){ /*GET N*/
        nread=read(newfd,ptr,1);
        if(nread <= 0) /*error*/{printf("Read Failed on POST\n");return 0;}
        nleft-=nread;
        ptr+=nread;
        }
    
    ptr = gid;
    remove_white_spaces(uid);
    printf("UID1 %s\n",uid);
    nleft = 3;
    while(nleft>0){ /*GET N*/
        nread=read(newfd,ptr,1);
        if(nread <= 0) /*error*/{printf("Read Failed on POST\n");return 0;}
        nleft-=nread;
        ptr+=nread;
    }
    remove_white_spaces(gid);
    printf("GID1 %s\n",gid);
    nleft = 4; ptr = tsize;
    while(nleft>0){
        nread=read(newfd,ptr,1);
        if (ptr[0] == ' '){
            nleft-=nread;
            ptr+=nread;
            break;
        }
        if(nread ==-1)/*error*/{printf("Read Failed on POST\n");return 0;}
        else if(nread==0)break;//closed by peer
        nleft-=nread;
        ptr+=nread; 
    }
     printf("TSIZE1 %s\n",tsize);
    nleft = atoi(tsize);ptr = text;
    while(nleft>0){
        nread=read(newfd,ptr,nleft);
        if(nread ==-1)/*error*/{printf("Read Failed on POST\n");return 0;}
        else if(nread==0)break;//closed by peer
        nleft-=nread;
        ptr+=nread; 
    }
    printf("Text1 %s\n",text);
     nleft = 1; ptr = spaces;
    while(nleft>0){
            nread=read(newfd,ptr,1);
            if(nread ==-1)/*error*/{printf("Read Failed on POST\n");return 0;}
            else if(nread==0)break;//closed by peer
            nleft-=nread;
            ptr+=nread; 
    }
    
    char islogged[30] = "";
    char gid_exist[30] = "";
    char issub[30] = "";
    char msg_dir_author[50]= "";
    char msg_dir_text[50]= "";
    Messages_in_group(gid,n_msg,1);
    sprintf(msg_dir,"GROUPS/%.2s/MSG/%.4s",gid,n_msg);
    sprintf(msg_dir_author,"%s/A U T H O R.txt",msg_dir);
    sprintf(msg_dir_text,"%s/T E X T.txt",msg_dir);
    sprintf(islogged,"USERS/%.5s/%.5s_login.txt",uid,uid);
    sprintf(gid_exist,"GROUPS/%.2s/%.2s_name.txt",gid,gid);
    sprintf(issub,"GROUPS/%.2s/%.5s.txt",gid,uid);
    FILE *f;
    if((access( islogged, F_OK ) != 0) || (access( gid_exist, F_OK ) != 0 ) || (access( issub, F_OK ) != 0)){
        flag = 1;
    }
    if (flag == 0){
        mkdir(msg_dir,0700);
        f = fopen(msg_dir_author,"w");
        fwrite(uid,sizeof(char),5,f);
        fclose(f);
        f = fopen(msg_dir_text,"w");
        fwrite(text,sizeof(char),strlen(text),f);
        fclose(f);
    }
    if(spaces[0] == ' '){
        if(access( islogged, F_OK ) != 0 || access( gid_exist, F_OK ) != 0 ||access( issub, F_OK ) != 0){
            flag = 2;
        }
        nleft = 25;ptr = filename;
            while(nleft>0){
                nread=read(newfd,ptr,1);
                if(nread ==-1)/*error*/{printf("Read Failed on POST\n");return 0;}
                else if(nread==0)break;//closed by peer
                    if (ptr[0] == ' '){
                    nleft-=nread;
                    ptr+=nread;
                    break;
                }
                nleft-=nread;
                ptr+=nread; 
            }
            remove_white_spaces(filename);
            
            
        nleft = 11;ptr = fsize;
            while(nleft>0){
                nread=read(newfd,ptr,1);
                if(nread ==-1)/*error*/{printf("Read Failed on POST\n");return 0;}
                else if(nread==0)break;//closed by peer
                    if (ptr[0] == ' '){
                    nleft-=nread;
                    ptr+=nread;
                    break;
                }
                nleft-=nread;
                ptr+=nread; 
        }
        char data[514];
        if(flag == 0){
            sprintf(file_path,"%s/%.24s",msg_dir,filename);
            f = fopen(file_path,"wb");
        }
            nleft= atoi(fsize);
            while(nleft > 0){
                ptr = data;
                if(nleft < 512){
                    nread = read(newfd,ptr,nleft);
                     if(nread ==-1)/*error*/{printf("Read Failed on POST\n");return 0;}
                    else if(nread==0)break;//closed by peer
                }
                else{
                    nread = read(newfd,ptr,512);
                     if(nread ==-1)/*error*/{printf("Read Failed on POST\n");return 0;}
                    else if(nread==0)break;//closed by peer
                }
                if (flag == 0){
                    fwrite(data,sizeof(unsigned char),nread,f);
                }
                nleft-=nread;
                ptr+=nread; 
                strcpy(data,"");   
            }
        if(flag == 0){
        fclose(f);
        }
    }
    if(flag == 0){
        char send[12]= "";
        sprintf(send, "RPT %s\n",n_msg);
        printf("send %s\n",send);
        nleft = 9; ptr = send;
            while(nleft>0){
                nwritten=write(newfd,ptr,nleft);
                if(nwritten ==-1)/*error*/{printf("Write Failed on POST\n");return 0;}
                else if(nwritten==0)break;//closed by peer
                nleft-=nwritten;
                ptr+=nwritten; 
            }
        return 0;
        if(flag_v == 1){
            printf("post group %.2s:\"%s\" %s\n",gid,text,filename);
        }
    }
    else{
        
        if(flag = 2){
            unlink(file_path);
        }
        rmdir(msg_dir);
        char send[12]= "";
        strcpy(send,"RPT NOK\n");
        nleft = strlen(send); ptr = send;
            while(nleft>0){
                nwritten=write(newfd,ptr,nleft);
                if(nwritten ==-1){printf("Write Failed on POST\n");return 0;}
                else if(nwritten==0)break;//closed by peer
                nleft-=nwritten;
                ptr+=nwritten; 
            }
        return 0;
    }
    return 0;
}


int TcpCommands(){
    char command[5] = "";
    char *ptr;
    ssize_t nbytes,nleft,nwritten,nread;
    ptr = command;
    nleft= 4; 
    while(nleft > 0){ /* Le o primeiro comando e o espaço a seguir*/
        nread=read(newfd,ptr,1);
        if(nread==-1){printf("Read failed getting command\n");break;}
        else if(nread==0)break;//closed by peer
        nleft-=nread;
        ptr+=nread;
    }
    if(strcmp(command,"ULS ") ==0 ){
        ulist();
    }
    if(strcmp(command,"PST ") ==0 ){
        post();
    }
    if(strcmp(command,"RTV ") == 0){
        char buffer3[20] = "";
        strcpy(buffer3,"RRT NOK\n");
        nleft = strlen(buffer3); ptr =buffer3;
        while(nleft > 0){ /* Le o primeiro comando e o espaço a seguir*/
        nread=write(newfd,ptr,1);
        if(nread==-1){printf("Read failed getting command\n");break;}
        else if(nread==0)break;//closed by peer
        nleft-=nread;
        ptr+=nread;
    }
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
    /*Connection*/
    memset(&hints_fd,0,sizeof hints_fd);
    hints_fd.ai_family=AF_INET;
    hints_fd.ai_socktype=SOCK_DGRAM;
    hints_fd.ai_flags=AI_PASSIVE;
    
    memset(&hints_tcp,0,sizeof hints_tcp);
    hints_tcp.ai_family=AF_INET; //IPv4
    hints_tcp.ai_socktype=SOCK_STREAM; //TCP socket
    hints_tcp.ai_flags=AI_PASSIVE;

    sprintf(dsport, "%d",dsport_err);
    errcode=getaddrinfo(NULL,dsport,&hints_fd,&res_udp);
    if(errcode != 0){ printf("Nao conseguiu obter endereço"); exit(1); }
    errcode=getaddrinfo(NULL,dsport,&hints_tcp,&res_tcp);
    if(errcode != 0) {printf("Nao conseguiu obter endereço"); exit(1); }
    
    udpfd = socket(AF_INET,SOCK_DGRAM,0) /*UDP*/;
    if(udpfd ==-1){ printf("Failed Udp Socket\n");  exit(1);}
    n= bind(udpfd,res_udp->ai_addr,res_udp->ai_addrlen);
    if(n==-1) {printf("Nao consegui dar bind udpfd\n");  exit(1);}

    tcpfd= socket(AF_INET,SOCK_STREAM,0) /*TCP*/;
    if(tcpfd ==-1) {printf("Failed Tcp socket"); exit(1);}
    n= bind(tcpfd,res_tcp->ai_addr,res_tcp->ai_addrlen);
    if(n==-1){ printf("Nao consegui dar bind tcpfd\n"); exit(1); }
    if(listen(tcpfd,5) == -1) {printf("Nao consegui dar listen tcpfd\n"); exit(1); } 

     
    int flag = 0;
    while(1){
        
        FD_ZERO(&rfds); 
        FD_SET(udpfd,&rfds);
        FD_SET(tcpfd,&rfds);
        maxfd=max(udpfd,tcpfd);
        checker=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
        if(checker <= 0) printf("Checker Failed\n");
        if(FD_ISSET(udpfd,&rfds)){ /*UDP */
            if (flag == 0){
                flag = 1;
                char buffer[128] = "";
                addrlen = sizeof(addr);
                n=recvfrom(udpfd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
                if(n==-1) printf("Failed Recvfrom\n");
                UdpCommands(buffer);
                flag = 0;
            }
            else{
                printf("Falhei\n");
            }
        }
        if(FD_ISSET(tcpfd,&rfds)){ /*TCP*/
           if(flag == 0){
                flag = 1; /* User has to wait before doing other thins */
                addrlen=sizeof(addr);
                if((newfd=accept(tcpfd,(struct sockaddr*)&addr,&addrlen))==-1)/*error*/printf("Failed Accept\n");
                TcpCommands();
                
            flag = 0;
           }
        }
    }/*END OF WHILE*/
    return 0;
}