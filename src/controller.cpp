/*
 *  MIXER - A real-time video mixing application
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of thin MIXER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
 *            Ignacio Contreras <ignacio.contreras@i2cat.net>
 */     


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <string>
#include <iostream>

#include "event.h"
#include "mixer.h"
#include "../config.h"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void initialize_action_mapping();
int get_socket(int port, int *sock);
int listen_socket(int sock, int *newsock);
void start_mixer(Jzon::Object rootNode, Jzon::Object *outputRootNode);
void stop_mixer(Jzon::Object *outputRootNode);
void send_and_close(string msg, int socket);

map<string,void(Mixer::*)(Jzon::Object*, Jzon::Object*)> commands;
Jzon::Object rootNode;
Jzon::Parser parser(rootNode);
Jzon::Object output_root_node;
Jzon::Writer writer(output_root_node, Jzon::NoFormat);
bool should_stop=false;
Mixer *m;
string action;



int main(int argc, char *argv[]){

    int sockfd, newsockfd, portno, n;
    char buffer[2048];
    struct timeval in_time;
     
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    portno = atoi(argv[1]);
    get_socket(portno, &sockfd);
    int ts;
    initialize_action_mapping();

    while(!should_stop) {
        if (listen_socket(sockfd, &newsockfd) == 0) {
            bzero(buffer,2048);
            rootNode.Clear();
            output_root_node.Clear();
            n = read(newsockfd, buffer, 2047);
            if (n < 0) {
                error("ERROR reading from socket");
            }

            parser.SetJson(buffer);

            if (parser.Parse()) {
                action = rootNode.Get("action").ToString();

                if (action.compare("start") == 0){
                    start_mixer(rootNode, &output_root_node);
                    writer.Write();
                    send_and_close(writer.GetResult(), newsockfd);
                    continue;

                } else if (action.compare("stop") == 0){
                    stop_mixer(&output_root_node);
                    writer.Write();
                    send_and_close(writer.GetResult(), newsockfd);
                    continue;

                } else if (action.compare("exit") == 0){
                    continue;
                }

                if (commands.count(action) <= 0){
                    output_root_node.Add("error","Wrong action, try it again!");
                    writer.Write();
                    send_and_close(writer.GetResult(), newsockfd);
                    continue;
                }

                if (m == NULL){
                    output_root_node.Add("error","Mixer is not running. Cannot perform action!");
                    writer.Write();
                    send_and_close(writer.GetResult(), newsockfd);
                    continue;
                }

                gettimeofday(&in_time, NULL);
                ts = in_time.tv_sec*1000000 + in_time.tv_usec + rootNode.Get("delay").ToInt()*1000000;
                m->push_event(new Event(commands[action], rootNode.Get("params"), ts, newsockfd));

            }
        }
    }
    close(sockfd);
    return 0; 
}

void initialize_action_mapping()
{
    commands["add_source"] = &Mixer::add_source;
    commands["remove_source"] = &Mixer::remove_source;

    commands["add_crop_to_source"] = &Mixer::add_crop_to_source;
    commands["modify_crop_from_source"] = &Mixer::modify_crop_from_source;
    commands["modify_crop_resizing_from_source"] = &Mixer::modify_crop_resizing_from_source;
    commands["enable_crop_from_source"] = &Mixer::enable_crop_from_source;
    commands["disable_crop_from_source"] = &Mixer::disable_crop_from_source;
    commands["remove_crop_from_source"] = &Mixer::remove_crop_from_source;

    commands["add_crop_to_layout"] = &Mixer::add_crop_to_layout;
    commands["modify_crop_from_layout"] = &Mixer::modify_crop_from_layout;
    commands["modify_crop_resizing_from_layout"] = &Mixer::modify_crop_resizing_from_layout;
    commands["remove_crop_from_layout"] = &Mixer::remove_crop_from_layout;
    
    commands["add_destination"] = &Mixer::add_destination;
    commands["remove_destination"] = &Mixer::remove_destination;
    
    commands["get_streams"] = &Mixer::get_streams;
    commands["get_layout"] = &Mixer::get_layout;
    commands["get_stats"] = &Mixer::get_stats;

}

void start_mixer(Jzon::Object rootNode, Jzon::Object *outputRootNode)
{
    if (m != NULL){
        outputRootNode->Add("error", "Mixer is already running");
        return;
    }

    int width = rootNode.Get("params").Get("width").ToInt();
    int height = rootNode.Get("params").Get("height").ToInt();
    int in_port = rootNode.Get("params").Get("input_port").ToInt();

    m = new Mixer(width, height, in_port);

    if (m == NULL){
        outputRootNode->Add("error", "Error initializing Mixer");
    }

    m->start();
    outputRootNode->Add("error", Jzon::null);

}

void stop_mixer(Jzon::Object *outputRootNode){
    if (m == NULL){
        outputRootNode->Add("error", "Mixer is not running");
        return;
    }
    
    m->stop();
    delete m;
    outputRootNode->Add("error", Jzon::null);
}

int get_socket(int port, int *sock){
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

int listen_socket(int sock, int *newsock) {
    socklen_t clilen;
    struct sockaddr cli_addr;
    listen(sock,5);
    clilen = sizeof(cli_addr);
    *newsock = accept(sock, (struct sockaddr *) &cli_addr, &clilen);
    if (*newsock < 0) 
        return -1;

    return 0;
}

void send_and_close(string result, int socket)
{
    const char* res = result.c_str();
    int n = write(socket,res,result.size());
    close(socket);
}