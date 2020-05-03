#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>          
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <sys/time.h>
#include<sys/select.h>
#include<signal.h>
#define BUFLEN 1024
int max_client =0;
void usage();
int check_argv_arg(char* arg);
int main(int argc,char* argv[]){
    if(argc<3){
        usage();
        return 1;
    }
    if(check_argv_arg(argv[1])==-1){
        //printf("your port is unvalid\n");
        usage();
        return 1;
    }
    if(check_argv_arg(argv[2])==-1){
        //printf("your naxClient is unvalid\n");
        usage();
        return 1;
    }
    int port = atoi(argv[1]);
    if(port==0){
        usage();
        return 1;
    }
    max_client=atoi(argv[2]);
    if(max_client<=0){
        usage();
        //printf("need to put maxClient bigger then zero\n");
        return 1;
    }
    
    int check=0;
    struct sockaddr_in serv_addr;
    int main_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(main_sockfd<0){
        perror("socket\n");
        exit(1);
    }
    serv_addr.sin_family =AF_INET;
    serv_addr.sin_addr.s_addr=INADDR_ANY;
    serv_addr.sin_port = htons(port);
    check = bind(main_sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if(check<0){
        perror("bind\n");
        exit(1);
    }       
    check = listen(main_sockfd,5);
    if(check==-1){
        perror("listen\n");
        exit(1);
    }   
    int activ_fd=0;
    char buffer[BUFLEN];
    memset(buffer,'\0',BUFLEN);
    int read_write =0, time_to_write=0;
    int fd,rc=0,new_client =0;;			/* original socket */
    fd_set rfds;
    fd_set wfds;
    fd_set cpy_rfds;
    FD_ZERO(&rfds);
    FD_ZERO(&cpy_rfds);
    FD_ZERO(&wfds);
    FD_SET(main_sockfd,&rfds);
    activ_fd++;
    int maxfd = main_sockfd;
    while(1){
        cpy_rfds=rfds;
        rc = select(maxfd+1,&cpy_rfds,&wfds,NULL,NULL);
        if(rc<0){
            perror("select\n");
            exit(1);
        }
        /*if main in the copy so we want to accecpt but need to check that active < max of client */
        if(FD_ISSET(main_sockfd,&cpy_rfds) && time_to_write==0 && activ_fd<=max_client){
            new_client =accept(main_sockfd,NULL,NULL);
            if(new_client<0){
                perror("accecpt\n");
                exit(1);
            }
            FD_SET(new_client ,&rfds);
            activ_fd++;
            printf("new client add to the Q\n");
            /*keep the maxfd on the top of the table*/
            if(maxfd<new_client){
                    maxfd=new_client;
            }
        }
        /*need to read no write*/
        if(time_to_write==0){
            /*go over the fds and check if somone want to write to server */
            for(fd = main_sockfd+1;fd<maxfd+1;fd++){
                /* fd in  rhe cpy so read from him*/
                if(FD_ISSET(fd,&cpy_rfds)){
                    printf("fd %d is ready to read\n", fd);
                    memset(buffer,'\0',BUFLEN); 
                    rc= read(fd,&buffer,BUFLEN);
                    if(rc==0){// so he is in only to unset 
                        close(fd);
                        FD_CLR(fd,&rfds);
                        activ_fd--;
                    } 
                    else if(rc>0){// he write soom
                        FD_SET(fd,&wfds);
                        for(int i = main_sockfd+1;i<maxfd+1;i++){// go over and all the activs fd put on wfds
                            if(FD_ISSET(i,&rfds)){
                                FD_SET(i,&wfds);
                            }
                        }
                        read_write=1;// time to write in the next iteration
                        break;
                        
                    }  
                    else{  
                        perror("read\n");
                        exit(1);
                    }
                }
            }
            if(read_write==1){
                time_to_write=1;
            }
        }
        else{// time to write
            for(int fd_w = main_sockfd+1; fd_w < maxfd+1; fd_w++)// go over active on wfds
            {
                if(FD_ISSET(fd_w,&wfds)){// if is in wfds
                    printf("fd %d is ready to write\n", fd_w); 
                    write(fd_w,buffer,strlen(buffer));// write  to the client
                    FD_CLR(fd_w,&wfds);// clean 
                }
            }
            read_write=0;
            time_to_write=0;
        }
    }
    return 0;
}
/*The function checks that the 
 numbers you receive are
 correct and indeed numbers and 
 not strings that are both numbers and letters*/
int check_argv_arg(char* arg){
    int fleg=0;
    for(int i = 0; i < strlen(arg); i++)
    {
        if(arg[i]>57 || arg[i]<48){
            fleg=-1;
        }
    }
    return fleg;
}
void usage(){
    printf("You should  put arg like this: ./chatserver <port> <max_clients>\n ");
}