#include <iostream>
#include <sys/time.h>
#include <queue>
#include <pthread.h>
#include "Jzon.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void* routine(void *arg);
int listen_socket(int sock, int *newsock);
int get_socket(int port, int *sock);

class Event
{
    public:
        int delay;
        string action;
        int socket;
        int timestamp;
        
        bool operator<(const Event& e) const
        {
            return timestamp > e.timestamp;
        }

        Event(string msg, int d, int ts, int s)
        {
            action = msg;
            delay = d;
            timestamp = ts;
            socket = s;
        }

        void exec_func()
        {
            ostringstream s;
            s << "Main method " << delay;
            string msg(s.str());
            const char* res = msg.c_str();
            write(socket, res, msg.size());
            cout << msg << endl;
            close(socket);
        }
};

Jzon::Object rootNode, root_response;
Jzon::Parser parser(rootNode);
bool should_stop=false;


int main ()
{
    priority_queue<Event> eventQueue;
    struct timeval in_time;
    int delay;
    string action;
    bool response;
    pthread_t thread;
    pthread_create(&thread, NULL, routine, &eventQueue);

    int sockfd, newsockfd, portno, n;
    const char* res;
    string result; 
    char buffer[2048];
     
    portno = 7777;
    get_socket(portno, &sockfd);

    while(!should_stop){
        if (listen_socket(sockfd, &newsockfd) == 0){
            bzero(buffer,2048);
            rootNode.Clear();
            root_response.Clear();
            n = read(newsockfd,buffer,2047);
            if (n < 0){
                error("ERROR reading from socket");
            }

            parser.SetJson(buffer);
            if (parser.Parse()){
                action = rootNode.Get("action").ToString();
                delay = rootNode.Get("delay").ToInt();
                gettimeofday(&in_time, NULL);
                eventQueue.push(Event(action, delay, in_time.tv_sec*1000000 + in_time.tv_usec + delay*1000000, newsockfd));
            }
        }
    }
    close(sockfd);
    return 0; 
}

void* routine(void *arg){
    priority_queue<Event>* eventQueue = ((priority_queue<Event>*)arg);
    struct timeval clock;
    int curr_ts;

    while (1){
        gettimeofday(&clock, NULL);
        curr_ts = clock.tv_sec*1000000 + clock.tv_usec;

        if (!eventQueue->empty() && eventQueue->top().timestamp <= curr_ts){ //TODO: this can be a while if we want to execute N orders in one iteration
            Event tmp = eventQueue->top();
            tmp.response();
            eventQueue->pop();
        }

        usleep(1000);
    }
}

int get_socket(int port, int *sock)
{
    struct sockaddr_in serv_addr;
    int yes=1;

    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0) 
        error("ERROR opening socket");
    if ( setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ){
        perror("setsockopt");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(*sock, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");

    return 0;     
}

int listen_socket(int sock, int *newsock) 
{
    socklen_t clilen;
    struct sockaddr cli_addr;
    listen(sock,5);
    clilen = sizeof(cli_addr);
    *newsock = accept(sock, (struct sockaddr *) &cli_addr, &clilen);
    if (*newsock < 0) 
        return -1;

    return 0;
}