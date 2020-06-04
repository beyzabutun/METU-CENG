#include <iostream>
#include <fstream>
#include "monitor.h"
#include<pthread.h>
#include <deque>
#include<unistd.h>
#include <vector>
#include <math.h>
#include <sys/time.h>


using namespace std;

struct PersonParams{
    int p_id;
    int weight_person;
    int initial_floor;
    int destination_floor;
    int priority;
    int num_of_floor;
    bool ex_try=false;
    bool first_try=true;
};
struct ElevatorParams{
    int num_floors;
    int weight_capacity;
    int person_capacity;
    int travel_time;
    int idle_time;
    int in_out_time;
    int num_people;
};
class Elevator:public Monitor{
    string state;
    int current_floor;
    int current_weight;
    int people_in_ele;
    int active_person;
    int ele_capacity;
    int weight_capacity;
    std::vector<PersonParams> eleVector;
    std::vector<int> prioVector;
    deque <int> destQueue;
    Condition is_idle;
    Condition ele_stopped;
    Condition enter_ele;
    Condition sleep;
    Condition lp_turn;

    public:
        Elevator(): is_idle(this), ele_stopped(this), enter_ele(this), sleep(this), lp_turn(this){
            current_floor = current_weight = people_in_ele = 0;
            state="Idle";
        }
        void prioVectorInit(int num_floor){
            for(int i=0;i<num_floor;i++){
                prioVector.push_back(0);
            }
        }
        void intervalwait(int sleep_time){
            timeval    tp;
            gettimeofday(&tp, NULL);
            timespec ts;
            ts.tv_sec  = tp.tv_sec+((tp.tv_usec+sleep_time) * 1000)/pow(10,9);
            ts.tv_nsec = fmod(((tp.tv_usec+sleep_time) * 1000),pow(10,9));
            sleep.timedWait(&ts);
        }
        void changeState(PersonParams &perParams){
            if(state=="Idle"){
                if(current_floor > perParams.initial_floor){
                    state="Moving-down";
                }
                else if(current_floor < perParams.initial_floor){
                    state="Moving-up";
                }
                else if(current_floor==perParams.initial_floor){
                    if(current_floor > perParams.destination_floor){
                        state="Moving-down";
                    }
                    if(current_floor < perParams.destination_floor){
                        state="Moving-up";
                    }
                }
            }
        }
        void printLeave(PersonParams &perParams){
            string prio;
            if(perParams.priority==1) prio="hp";
            else prio="lp";
            cout<<"Person ("<<perParams.p_id<<", "<<prio<<", "<<perParams.initial_floor<<" -> "<<perParams.destination_floor<<", "
            <<perParams.weight_person<<") has left the elevator"<<endl;
        }
        void printEnter(PersonParams &perParams){
            string prio;
            if(perParams.priority==1) prio="hp";
            else prio="lp";
            cout<<"Person ("<<perParams.p_id<<", "<<prio<<", "<<perParams.initial_floor<<" -> "<<perParams.destination_floor<<", "
            <<perParams.weight_person<<") entered the elevator"<<endl;
        }
        void printRequest(PersonParams &perParams){
            string prio;
            if(perParams.priority==1) prio="hp";
            else prio="lp";
            cout<<"Person ("<<perParams.p_id<<", "<<prio<<", "<<perParams.initial_floor<<" -> "<<perParams.destination_floor<<", "
            <<perParams.weight_person<<") made a request"<<endl;
        }
        void printEleInfo(){
            cout<<"Elevator ("<<state<<", "<<current_weight<<", "<<people_in_ele<<", "<<current_floor<<" ->";
            std::deque<int>::iterator it;
            if(state=="Moving-up"){
                for (it=destQueue.begin(); it!=destQueue.end(); ++it){
                    if(it==destQueue.begin()) cout<<" ";
                    if(it!=destQueue.begin()) cout<<",";
                    cout<<*it;
                }
            }
            else if(state=="Moving-down"){
                for (it=destQueue.end()-1; it!=destQueue.begin()-1; --it){
                    if(it==destQueue.end()-1) cout<<" ";
                    if(it!=destQueue.end()-1) cout<<",";
                    cout<<*it;
                }
            }   
            cout<<")"<<endl;
        }
        void insertFloor(int floor){
            if(destQueue.empty()){
                destQueue.push_back(floor);
            }
            else{
                std::deque<int>::iterator it;
                for (it=destQueue.begin(); it!=destQueue.end(); ++it){
                    if(floor==*it) return;
                    else if(floor<*it){
                        destQueue.insert(it, floor);
                        return;
                    } 
                }
                destQueue.push_back(floor);       
            }
        }
        bool isEligible(PersonParams &perParams){
            //__synchronized__;
            int init_floor=perParams.initial_floor;
            int dest_floor=perParams.destination_floor;
            if(state=="Idle"){
                return true;
            }
            else if(state=="Moving-up" and init_floor>=current_floor and dest_floor>init_floor){
                return true;
            }
            else if(state=="Moving-down" and init_floor<=current_floor and dest_floor<init_floor){
                return true;
            }
            else return false;
        }
        bool dontEnter(PersonParams &perParams, string stat){
            if(stat=="Moving-up" and perParams.initial_floor>perParams.destination_floor) return true;
            if(stat=="Moving-down" and perParams.initial_floor<perParams.destination_floor) return true;
            return false;
        }
        void makeRequest(PersonParams &perParams){
            __synchronized__;
            while (!isEligible(perParams) or perParams.ex_try==true){
                is_idle.wait();
                perParams.ex_try=false;
            }
            if(current_floor!=perParams.initial_floor) insertFloor(perParams.initial_floor);
            changeState(perParams);
            printRequest(perParams);
            printEleInfo();
        }
        bool inProcess(PersonParams &perParams){
            __synchronized__;
            bool canEnter = true;
            int init_floor=perParams.initial_floor;
            while(current_floor!=init_floor){
                ele_stopped.wait();
                if(state=="Idle"){
                    return false;
                }
            }
            for (int i=0; i<eleVector.size(); ++i){
                if(eleVector[i].destination_floor==current_floor){
                    canEnter=false;
                }
            }
            if(canEnter==false) enter_ele.wait();
            if(perParams.priority==1){

                if((people_in_ele+1)>ele_capacity or
                (current_weight+perParams.weight_person)>weight_capacity or dontEnter(perParams, state))  {
                    perParams.ex_try=true;
                    perParams.first_try=false;
                    int count=0;
                    for (int i=0; i<eleVector.size(); ++i){
                        if(eleVector[i].priority==1 and eleVector[i].initial_floor==perParams.initial_floor){
                            count++;
                        }
                    }
                    if((count+1)==prioVector[perParams.initial_floor]) lp_turn.notifyAll();
                    return false;
                }
                prioVector[perParams.initial_floor]=prioVector[perParams.initial_floor]-1;
                eleVector.push_back(perParams);
                people_in_ele=people_in_ele+1;
                current_weight=current_weight+perParams.weight_person; 
                insertFloor(perParams.destination_floor);
                changeState(perParams);
                if(prioVector[perParams.initial_floor]==0){
                    lp_turn.notifyAll();
                }
            }
            else if(perParams.priority==2){
                if(prioVector[perParams.initial_floor]>0){
                    lp_turn.wait();
                }
                if((current_weight+perParams.weight_person)>weight_capacity or
                (people_in_ele+1)>ele_capacity or dontEnter(perParams, state)) {
                    perParams.ex_try=true;
                    perParams.first_try=false;
                    return false;
                }
                eleVector.push_back(perParams);
                people_in_ele=people_in_ele+1;
                current_weight=current_weight+perParams.weight_person; 

                insertFloor(perParams.destination_floor);
                changeState(perParams);
            }
            
            printEnter(perParams);
            printEleInfo();
            return true;
        }
        void outProcess(PersonParams &perParams){
            __synchronized__;
            bool canEnter = true;
            int dest_floor=perParams.destination_floor;
            while(current_floor!=dest_floor){
                ele_stopped.wait();
            }
            active_person=active_person-1;
            people_in_ele=people_in_ele-1;
            current_weight=current_weight-perParams.weight_person; 
            for (int i=0; i<eleVector.size(); ++i){
                if(eleVector[i].p_id==perParams.p_id){
                    eleVector.erase(eleVector.begin()+i);
                }
            }
            for (int i=0; i<eleVector.size(); ++i){
                if(eleVector[i].destination_floor==current_floor){
                    canEnter=false;
                    break;
                }
            }
            if(canEnter==true) enter_ele.notifyAll();
            printLeave(perParams);
            printEleInfo();
        }
        void personProcess(PersonParams &perParams){
            makeRequest(perParams);
            if(perParams.first_try==true and perParams.priority==1) prioVector[perParams.initial_floor]+=1;
            bool is_handled = inProcess(perParams);
            if(is_handled==true){
                outProcess(perParams);
            }
            else personProcess(perParams);
        }
        void elevatorProcess(ElevatorParams &eleParams){
            __synchronized__;
            active_person=eleParams.num_people;
            ele_capacity=eleParams.person_capacity;
            weight_capacity=eleParams.weight_capacity;
            while(active_person){
                if(state=="Idle"){
                    is_idle.notifyAll();
                    while(1){
                        intervalwait(eleParams.idle_time);
                        if(!destQueue.empty()) break;  
                    }
                }
                else if(state=="Moving-up"){
                    int next_floor=destQueue.front();
                    while(current_floor<eleParams.num_floors){
                        intervalwait(eleParams.travel_time);    
                        current_floor=current_floor+1;
                        if(current_floor==next_floor){
                            destQueue.pop_front();
                            if(destQueue.size()==0){
                                state="Idle";
                                is_idle.notifyAll();
                            }
                            printEleInfo();
                            ele_stopped.notifyAll();
                            intervalwait(eleParams.in_out_time);
                            break;
                        }
                        printEleInfo();
                    }
                }
                else{ //Moving-down
                    int next_floor=destQueue.back();
                    while(current_floor!=0){
                        intervalwait(eleParams.travel_time);
                        current_floor=current_floor-1;
                        if(current_floor==next_floor){
                            destQueue.pop_back();
                            if(destQueue.size()==0){
                                state="Idle";
                                is_idle.notifyAll();
                            }
                            printEleInfo();
                            ele_stopped.notifyAll();
                            intervalwait(eleParams.in_out_time);
                            break;
                        }
                        printEleInfo();
                    }
                    
                }
            }
        }
};

Elevator ele;

void *person_thread(void *perParams){
    PersonParams *params = (PersonParams *)perParams;
    if(params->p_id==0) ele.prioVectorInit(params->num_of_floor);
    ele.personProcess(*params);
}
void *elevator_controller(void *eleParams){
    ElevatorParams *params = (ElevatorParams *)eleParams;
    ele.elevatorProcess(*params);
}

int main(int argc, const char* argv[]){
    pthread_t *person_threads, elevator_cont_thread;
    int num_floors, num_people, weight_capacity, person_capacity;
    int travel_time, idle_time, in_out_time;
    int weight_person, initial_floor, destination_floor, priority;
    ifstream inpFile;

    //PARSER    
    inpFile.open(argv[1]);
    inpFile>>num_floors>>num_people>>weight_capacity>>person_capacity;
    inpFile>>travel_time>>idle_time>>in_out_time;
    //END_PARSER

    person_threads = new pthread_t[num_people];
    PersonParams *perParams = new PersonParams[num_people];
    ElevatorParams *eleParams = new ElevatorParams;

    eleParams->num_floors=num_floors;           eleParams->weight_capacity=weight_capacity;
    eleParams->person_capacity=person_capacity; eleParams->travel_time=travel_time;
    eleParams->idle_time=idle_time;             eleParams->in_out_time=in_out_time;
    eleParams->num_people=num_people;
    pthread_create(&elevator_cont_thread, NULL, elevator_controller, (void*)eleParams);
    

    for(int i=0; i<num_people; i++){
        inpFile>>weight_person>>initial_floor>>destination_floor>>priority;
        perParams[i].p_id=i; 
        perParams[i].weight_person = weight_person;
        perParams[i].initial_floor = initial_floor;
        perParams[i].destination_floor = destination_floor;
        perParams[i].priority = priority;
        perParams[i].num_of_floor = num_floors;
        pthread_create(&person_threads[i], NULL, person_thread, (void *)(perParams + i));
    }
    pthread_join(elevator_cont_thread, NULL);
    for (int i = 0; i < num_people; i++) {
        pthread_join(person_threads[i], NULL);
    }


    delete[] person_threads;
    delete[] eleParams;
    delete[] perParams;

    return 0;


}