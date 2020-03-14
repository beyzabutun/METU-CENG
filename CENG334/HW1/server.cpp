#include<sys/socket.h>
#include<poll.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <iostream>
#include<sys/select.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<vector>
#include<array>
#include <string.h>
#include <cstring>
#include "message.h"
#include "logging.h"
#include "logging.c"
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
    cm message;
    int timeout = 0;
    int bidder_status;
    int statuses[n_of_bidders];

    //MESSAGE_DECS
    int delay;
    int curr_bid = starting_bid;
    int winner_bidder_id;
    int n_of_active_bidder = n_of_bidders;
    //
    
    for(int i=0; i<n_of_bidders;i++){
        pfd[i] = (pollfd){fd[i][0], POLLIN, 0};
        close(fd[i][1]);
    }
   
    while(check_negativity(pfd, n_of_bidders)){
        
        if(n_of_active_bidder==0){
            print_server_finished(winner_bidder_id, curr_bid);
            break;
        }
        for  (int i=0; i<n_of_bidders; i++){
            pfd[i].revents = 0;
        } 	
        poll(pfd, n_of_bidders, timeout);
        for(int i=0; i<n_of_bidders; i++){
            if(pfd[i].revents && POLLIN){
                r = read(pfd[i].fd, &message, sizeof(cm));
                if(r==0){
                    pfd[i].fd = -1;
                }
                else{
                    if(message.message_id == CLIENT_CONNECT){
                        ii inp_data;
                        oi out_data;
                        sm mess_sent;
                        delay = message.params.delay;
                        if(timeout==0){
                            timeout=delay;
                        }
                        if(timeout != 0){
                            if(timeout>delay){
                                timeout = delay;
                            }
                        }
                        //print input
                        inp_data.type = CLIENT_CONNECT;
                        inp_data.pid = pidList[i];
                        inp_data.info.delay = delay;
                        print_input(&inp_data, i);
                        //
                        mess_sent.message_id = SERVER_CONNECTION_ESTABLISHED;
                        mess_sent.params.start_info.client_id = i;
                        mess_sent.params.start_info.current_bid = curr_bid;
                        mess_sent.params.start_info.minimum_increment = minimum_increment;
                        mess_sent.params.start_info.starting_bid = starting_bid;
                        write(pfd[i].fd, &mess_sent, sizeof(sm));
                        //print output
                        out_data.type = SERVER_CONNECTION_ESTABLISHED;
                        out_data.pid = pidList[i];
                        out_data.info.start_info.client_id = i;
                        out_data.info.start_info.current_bid = curr_bid;
                        out_data.info.start_info.minimum_increment = minimum_increment;
                        out_data.info.start_info.starting_bid = starting_bid;
                        print_output(&out_data, i);
                        //
                    }
                    else if(message.message_id == CLIENT_BID){
                        ii inp_data;
                        oi out_data;
                        sm mess_sent;
                        mess_sent.message_id = SERVER_BID_RESULT;
                        int bid = message.params.bid;
                        //print input
                        inp_data.type = message.message_id;
                        inp_data.pid = pidList[i];
                        inp_data.info.bid = bid;
                        print_input(&inp_data, i);
                        //
                        if(bid<starting_bid){
                            mess_sent.params.result_info.result = BID_LOWER_THAN_STARTING_BID;
                        }
                        else if(bid<curr_bid){
                            mess_sent.params.result_info.result = BID_LOWER_THAN_CURRENT;
                        }
                        else if((bid-curr_bid)<minimum_increment){
                            mess_sent.params.result_info.result = BID_INCREMENT_LOWER_THAN_MINIMUM;
                        }
                        else if(bid>curr_bid){
                            curr_bid=bid;
                            winner_bidder_id = i;
                            mess_sent.params.result_info.result = BID_ACCEPTED;
                        }  
                        mess_sent.params.result_info.current_bid = curr_bid;
                        write(pfd[i].fd, &mess_sent, sizeof(sm));
                        //print output
                        out_data.type = SERVER_BID_RESULT;
                        out_data.pid = pidList[i];
                        out_data.info.result_info.current_bid = curr_bid;
                        out_data.info.result_info.result = mess_sent.params.result_info.result;
                        print_output(&out_data, i);
                    }
                    else if(message.message_id == CLIENT_FINISHED){
                        ii inp_data;
                        int status = message.params.status;
                        statuses[i] = status;
                        //print input
                        inp_data.type = CLIENT_FINISHED;
                        inp_data.pid = pidList[i];
                        inp_data.info.status = status;
                        print_input(&inp_data, i);
                        //
                        n_of_active_bidder-=1;
                        
                    }
                }
            }
        }			
    }
    sm mess_sent;
    oi out_data;
    mess_sent.message_id = SERVER_AUCTION_FINISHED;
    mess_sent.params.winner_info.winner_id = winner_bidder_id;
    mess_sent.params.winner_info.winning_bid = curr_bid;
    out_data.type = SERVER_AUCTION_FINISHED;
    out_data.info.winner_info.winner_id = winner_bidder_id;
    out_data.info.winner_info.winning_bid = curr_bid;
    for(int i=0;i<n_of_bidders;i++){
        out_data.pid = pidList[i];
        write(pfd[i].fd, &mess_sent, sizeof(sm));
        print_output(&out_data, i);
    }
    for(int i=0; i<n_of_bidders; i++){
        pid_t wpid = wait(&bidder_status);
        if(statuses[i] == bidder_status){
            print_client_finished(i, statuses[i], 1);
        }
        else{
            print_client_finished(i, statuses[i], 0);
        }
        close(pfd[i].fd);
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
        int pid = fork();
        if(pid==0){
            //child
            dup2(fd[i][1],STDIN_FILENO);
            dup2(fd[i][1],STDOUT_FILENO);
            close(fd[i][0]);

            int n_of_args = args.at(i).at(0);
            char *arr[n_of_args+2];
            arr[0] = new char[paths.at(i).size()];
            strcpy(arr[0], paths.at(i).c_str() );
            
            for(int j=0; j<n_of_args; j++){
                std::string s = std::to_string(args.at(i).at(j+1));
                arr[j+1]= new char(s.size());
                strcpy(arr[j+1], s.c_str());
            }
            arr[n_of_args+1] = (char *)NULL;
            execv(arr[0],arr);
   
            return 0;
        }
        else{
            //parent
            pidList[i]=pid; 
        }
    }  
    server(fd, pidList, starting_bid, minimum_increment, n_of_bidders); 
    return 0;
}