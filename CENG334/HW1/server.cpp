#include<sys/socket.h>
#include<poll.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <iostream>
#include<sys/select.h>
#include<sys/wait.h>
#include<sys/time.h>
#include "../lib/logging.h"
#include<vector>
#include<array>
#include <typeinfo>
#include <string.h>
#include <fstream>
#include <cstring>
#include "../lib/message.h"
#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

bool check_negativity(pollfd pfd[], int n_of_bidders){
    for(int i=0; i<n_of_bidders; i++){
        if(pfd[i].fd >= 0){
            return true;
        }  
    }
    return false;
}

void server(int fd[][2],int pidList[], int starting_bid, int minimum_increment, int n_of_bidders){
    pollfd pfd[n_of_bidders] = {};
    int r;
    client_message message;
    int timeout = 0;

    //MESSAGE_DECS
    int delay;
    int curr_bid = starting_bid;
    //
    
    for(int i=0; i<n_of_bidders;i++){
        pfd[i] = (pollfd){fd[i][0], POLLIN, 0};
        close(fd[i][1]);
    }
   
    while(check_negativity(pfd, n_of_bidders)){
        for  (int i=0; i<n_of_bidders; i++){
            pfd[i].revents = 0;
        } 	
        poll(pfd, n_of_bidders, timeout);
        for(int i=0; i<n_of_bidders; i++){
            if(pfd[i].revents && POLLIN){
                r = read(pfd[i].fd, &message, sizeof(client_message));
                if(r==0){
                    pfd[i].fd = -1;
                }
                else{
                    std::cout<<message.message_id<<std::endl;

                    //print_input(data,i);
                    if(message.message_id == CLIENT_CONNECT){
                        delay = message.params.delay;
                        if(timeout==0){
                            timeout=delay;
                        }
                        if(timeout != 0){
                            if(timeout>delay){
                                timeout = delay;
                            }
                        }
                        sm mess_sent;
                        mess_sent.message_id = SERVER_CONNECTION_ESTABLISHED;
                        mess_sent.params.start_info.client_id = i;
                        mess_sent.params.start_info.current_bid = curr_bid;
                        mess_sent.params.start_info.minimum_increment = minimum_increment;
                        mess_sent.params.start_info.starting_bid = starting_bid;
                        write(pfd[i].fd, &mess_sent, sizeof(sm));
                        std::cout<<"success"<<std::endl;

                    }
                    else if(message.message_id == CLIENT_BID){
                        std::cout<<"holllla"<<std::endl;

                    }
                    
                }
                
                
            }
        }			
    }
}


int main(int argc, const char* argv[]) {
    int starting_bid;
    int minimum_increment;
    int n_of_bidders;
    
    int i, n_of_args, arg;
    int n_of_inputs = 4;
    
    std::vector<std::string> paths;
    std::string path;
    std::vector<std::vector<int>> args;

    //PARSER
    std::cin >> starting_bid >> minimum_increment >> n_of_bidders;
    int fd[n_of_bidders][2];
    int pidList[n_of_bidders];
    pollfd pfd[n_of_bidders];
    
    for(int j=0; j<n_of_bidders; j++){
        std::vector<int> args_of_bidder;
        std::cin >> path >> n_of_args;
        
        paths.push_back(path);
        
        args_of_bidder.push_back(n_of_args);
        for(int i=0; i<n_of_args; i++){
            std::cin >> arg;
            args_of_bidder.push_back(arg);
        }
        args.push_back(args_of_bidder);
    }
    ////END OF PARSER

    for(int i=0; i<n_of_bidders;i++){
        PIPE(fd[i]);
    }
    for(i=0; i<n_of_bidders; i++){
        if(fork()==0){
            //child
            pidList[i]=getpid();
            dup2(fd[i][1],STDIN_FILENO);
            dup2(fd[i][1],STDOUT_FILENO);
            close(fd[i][0]);

            int n_of_args = args.at(i).at(0);
            char *arr[n_of_args+2];
            arr[0] = (char *)paths.at(i).c_str();
            for(int j=0; j<n_of_args; j++){
                std::string s = std::to_string(args.at(i).at(j+1));
                arr[j+1]=(char *)s.c_str();
            }
            arr[n_of_args+1] = (char *)NULL;
            
            execv(arr[0],arr);
   
            return 0;
    
        }
        //else{
            //close(fd[i][1]);
            //std::cout<<"parent"<<getpid()<<std::endl;  
            //parent
        //}
    }  
    server(fd, pidList, starting_bid, minimum_increment, n_of_bidders); 
    
    
    
    return 0;

}