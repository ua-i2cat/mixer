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

#include "Jzon.h"
#include "mixer.h"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void start_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode);
void stop_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode);

void add_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void remove_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void add_crop_to_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void modify_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void modify_crop_resizing_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void remove_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);

void add_crop_to_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);
void modify_crop_from_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);
void modify_crop_resizing_from_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);
void remove_crop_from_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);

void enable_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void disable_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);

void add_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);
void remove_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);

void get_streams(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_layout_size(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_state(Jzon::Object rootNode, Jzon::Object *outRootNode);
void exit_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode);

void initialize_action_mapping();
int get_socket(int port, int *sock);
int listen_socket(int sock, int *newsock);
int check_stream_id(uint32_t id);

std::map<std::string, void(*)(Jzon::Object, Jzon::Object*)> commands;
Jzon::Object rootNode, root_response;
Jzon::Parser parser(rootNode);
Jzon::Writer writer(root_response, Jzon::NoFormat);
bool should_stop=false;
Mixer *m;

int main(int argc, char *argv[]){

    int sockfd, newsockfd, portno, n;
    const char* res;
    std::string result; 
    char buffer[256];
     
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    portno = atoi(argv[1]);
    get_socket(portno, &sockfd);

    initialize_action_mapping();
    m = Mixer::get_instance();
    m->set_state(0);

    while(!should_stop){
    	if (listen_socket(sockfd, &newsockfd) == 0){
    		bzero(buffer,256);
        	rootNode.Clear();
        	root_response.Clear();
        	n = read(newsockfd,buffer,255);
        	if (n < 0) error("ERROR reading from socket");
        	std::cout << "Buffer_in:" << buffer << std::endl;
        	parser.SetJson(buffer);
        	if (!parser.Parse()) {
            	std::cout << "Error: " << parser.GetError() << std::endl;
        	}else { 
            	commands[rootNode.Get("action").ToString()](rootNode, &root_response);
            	writer.Write();
            	result = writer.GetResult();
            	std::cout << "Result:" << result << std::endl;
            	res = result.c_str();
            	n = write(newsockfd,res,result.size());
        	}
        close(newsockfd);
    	}
    }
    close(sockfd);
    return 0; 
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


void initialize_action_mapping() {
    commands["start_mixer"] = start_mixer;
    commands["stop_mixer"] = stop_mixer;

    commands["add_stream"] = add_stream;
    commands["remove_stream"] = remove_stream;
    commands["add_crop_to_stream"] = add_crop_to_stream;
    commands["modify_crop_from_stream"] = modify_crop_from_stream;
    commands["modify_crop_resizing_from_stream"] = modify_crop_resizing_from_stream;
    commands["remove_crop_from_stream"] = remove_crop_from_stream;

    commands["add_crop_to_layout"] = add_crop_to_layout;
    commands["modify_crop_from_layout"] = modify_crop_from_layout;
    commands["modify_crop_resizing_from_layout"] = modify_crop_resizing_from_layout;
    commands["remove_crop_from_layout"] = remove_crop_from_layout;

    commands["enable_crop_from_stream"] = enable_crop_from_stream;
    commands["disable_crop_from_stream"] = disable_crop_from_stream;
    
    commands["add_destination"] = add_destination;
    commands["remove_destination"] = remove_destination;
    
    commands["get_streams"] = get_streams;
    commands["get_layout"] = get_layout;
    commands["get_layout_size"] = get_layout_size;
    commands["get_state"] = get_state;
    commands["exit_mixer"] = exit_mixer;
}

void start_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    m = Mixer::get_instance();
    if (m->get_state() == 1){
        outRootNode->Add("error", "Mixer is already running");   
        return;
    }

    int width = rootNode.Get("params").Get("width").ToInt();
    int height = rootNode.Get("params").Get("height").ToInt();
    int in_port = rootNode.Get("params").Get("input_port").ToInt();
    m->init(width, height, in_port); 
    m->exec();
    m->set_state(1);
    outRootNode->Add("error", Jzon::null);  
}

void stop_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running. Cannot stop it");
        return;
    }
    
    m->stop();
    m->set_state(0);
    outRootNode->Add("error", Jzon::null);
}

void add_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int id = m->add_source();
        
    if (id == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("id", id);
    }
}

void remove_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

   int id = rootNode.Get("params").Get("id").ToInt();

    if (m->remove_source(id) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void add_crop_to_stream(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int id = rootNode.Get("params").Get("id").ToInt();
    int crop_width = rootNode.Get("params").Get("crop_width").ToInt();
    int crop_height = rootNode.Get("params").Get("crop_height").ToInt();
    int crop_x = rootNode.Get("params").Get("crop_x").ToInt();
    int crop_y = rootNode.Get("params").Get("crop_y").ToInt();
    int layer = rootNode.Get("params").Get("layer").ToInt();
    int rsz_width = rootNode.Get("params").Get("rsz_width").ToInt();
    int rsz_height = rootNode.Get("params").Get("rsz_height").ToInt();
    int rsz_x = rootNode.Get("params").Get("rsz_x").ToInt();
    int rsz_y = rootNode.Get("params").Get("rsz_y").ToInt();
    
    if (m->add_crop_to_source(id, crop_width, crop_height, crop_x, crop_y, layer, rsz_width, rsz_height, rsz_x, rsz_y) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void modify_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int stream_id = rootNode.Get("params").Get("stream_id").ToInt();
    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();
    int new_crop_width = rootNode.Get("params").Get("width").ToInt();
    int new_crop_height = rootNode.Get("params").Get("height").ToInt();
    int new_crop_x = rootNode.Get("params").Get("x").ToInt();
    int new_crop_y = rootNode.Get("params").Get("y").ToInt();

    if (m->modify_crop_from_source(stream_id, crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void modify_crop_resizing_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int stream_id = rootNode.Get("params").Get("stream_id").ToInt();
    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();
    int new_crop_width = rootNode.Get("params").Get("width").ToInt();
    int new_crop_height = rootNode.Get("params").Get("height").ToInt();
    int new_crop_x = rootNode.Get("params").Get("x").ToInt();
    int new_crop_y = rootNode.Get("params").Get("y").ToInt();
    int new_layer = rootNode.Get("params").Get("layer").ToInt();

    if (m->modify_crop_resizing_from_source(stream_id, crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y, new_layer) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void remove_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int stream_id = rootNode.Get("params").Get("stream_id").ToInt();
    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();

    if (m->remove_crop_from_source(stream_id, crop_id) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void add_crop_to_layout(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int crop_width = rootNode.Get("params").Get("width").ToInt();
    int crop_height = rootNode.Get("params").Get("height").ToInt();
    int crop_x = rootNode.Get("params").Get("x").ToInt();
    int crop_y = rootNode.Get("params").Get("y").ToInt();
    int output_width = rootNode.Get("params").Get("output_width").ToInt();
    int output_height = rootNode.Get("params").Get("output_height").ToInt();

    if (m->add_crop_to_layout(crop_width, crop_height, crop_x, crop_y, output_width, output_height) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void modify_crop_from_layout(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();
    int new_crop_width = rootNode.Get("params").Get("width").ToInt();
    int new_crop_height = rootNode.Get("params").Get("height").ToInt();
    int new_crop_x = rootNode.Get("params").Get("x").ToInt();
    int new_crop_y = rootNode.Get("params").Get("y").ToInt();

    if (m->modify_crop_from_layout(crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void modify_crop_resizing_from_layout(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();
    int new_width = rootNode.Get("params").Get("width").ToInt();
    int new_height = rootNode.Get("params").Get("height").ToInt();

    if (m->modify_crop_resizing_from_layout(crop_id, new_width, new_height) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void remove_crop_from_layout(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();

    if (m->remove_crop_from_layout(crop_id) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void enable_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int stream_id = rootNode.Get("params").Get("stream_id").ToInt();
    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();

    if (m->enable_crop_from_source(stream_id, crop_id) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void disable_crop_from_stream(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int stream_id = rootNode.Get("params").Get("stream_id").ToInt();
    int crop_id = rootNode.Get("params").Get("crop_id").ToInt();

    if (m->disable_crop_from_source(stream_id, crop_id) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void add_destination(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int stream_id = rootNode.Get("params").Get("stream_id").ToInt();
    std::string ip_string = rootNode.Get("params").Get("ip").ToString();
    uint32_t port = rootNode.Get("params").Get("port").ToInt();
    char *ip = new char[ip_string.length() + 1];
    strcpy(ip, ip_string.c_str());
    
    if (m->add_destination(ip, port, stream_id) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void remove_destination(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    uint32_t id = rootNode.Get("params").Get("id").ToInt();
    outRootNode->Add("error", Jzon::null);
    if (m->remove_destination(id) == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
    }
}

void get_streams(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    Jzon::Array stream_list;
    if(m->get_layout()->get_streams().empty()){
        outRootNode->Add("input_streams", stream_list);
        return;
    }

    std::map<uint32_t, Stream*> stream_map;
    std::map<uint32_t, Crop*> crop_map;
    std::map<uint32_t, Stream*>::iterator stream_it;
    std::map<uint32_t, Crop*>::iterator crop_it;

    stream_map = m->get_layout()->get_streams();
    
    for (stream_it = stream_map.begin(); stream_it != stream_map.end(); stream_it++){
        Jzon::Object stream;
        Jzon::Array crop_list;
        stream.Add("id", (int)stream_it->second->get_id());
        stream.Add("width", (int)stream_it->second->get_width());
        stream.Add("height", (int)stream_it->second->get_height());
        crop_map = stream_it->second->get_crops();
        for (crop_it = crop_map.begin(); crop_it != crop_map.end(); crop_it++){
            Jzon::Object crop;
            crop.Add("id", (int)crop_it->second->get_id());
            crop.Add("c_w", (int)crop_it->second->get_crop_width());
            crop.Add("c_h", (int)crop_it->second->get_crop_height());
            crop.Add("c_x", (int)crop_it->second->get_crop_x());
            crop.Add("c_y", (int)crop_it->second->get_crop_y());
            crop.Add("dst_w", (int)crop_it->second->get_dst_width());
            crop.Add("dst_h", (int)crop_it->second->get_dst_height());
            crop.Add("dst_x", (int)crop_it->second->get_dst_x());
            crop.Add("dst_y", (int)crop_it->second->get_dst_y());
            crop.Add("layer", (int)crop_it->second->get_layer());
            crop.Add("state", (int)crop_it->second->is_active());
            crop_list.Add(crop);
        }
        stream.Add("crops", crop_list);
        stream_list.Add(stream);
    }
    outRootNode->Add("input_streams", stream_list);
}

void get_layout(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    Jzon::Array crop_list;
    std::map<uint32_t, Crop*>::iterator crop_it;
    std::map<uint32_t, Crop*> crp = m->get_layout()->get_out_stream()->get_crops();
    std::vector<Mixer::Dst>::iterator dst_it;
    std::vector<Mixer::Dst> dst;

    Jzon::Object stream;
    stream.Add("id", (int)m->get_layout()->get_out_stream()->get_id());
    stream.Add("width", (int)m->get_layout()->get_out_stream()->get_width());
    stream.Add("height", (int)m->get_layout()->get_out_stream()->get_height());
    for (crop_it = crp.begin(); crop_it != crp.end(); crop_it++){
        Jzon::Object crop;
        Jzon::Array dst_list;
        crop.Add("id", (int)crop_it->second->get_id());
        crop.Add("c_w", (int)crop_it->second->get_crop_width());
        crop.Add("c_h", (int)crop_it->second->get_crop_height());
        crop.Add("c_x", (int)crop_it->second->get_crop_x());
        crop.Add("c_y", (int)crop_it->second->get_crop_y());
        crop.Add("dst_w", (int)crop_it->second->get_dst_width());
        crop.Add("dst_h", (int)crop_it->second->get_dst_height());

        dst = m->get_output_stream_destinations(crop_it->second->get_id());
        for (dst_it = dst.begin(); dst_it != dst.end(); dst_it++){
            Jzon::Object dst;
            dst.Add("id", (int)dst_it->id);
            dst.Add("ip", dst_it->ip);
            dst.Add("port", (int)dst_it->port);
            dst_list.Add(dst);
        }
        crop.Add("destinations", dst_list);
        crop_list.Add(crop);
    }
    stream.Add("crops", crop_list);
    outRootNode->Add("output_stream", stream);
}

void get_layout_size(Jzon::Object rootNode, Jzon::Object *outRootNode)
{
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    int width, height;

    width = m->get_layout_width();
    height = m->get_layout_height();

    outRootNode->Add("width", width);
    outRootNode->Add("height", height);

}


void get_state(Jzon::Object rootNode, Jzon::Object *outRootNode){
    uint8_t state = m->get_state();
    if (state == 1 || state == 0){
        outRootNode->Add("state", state);
    }else{
        outRootNode->Add("error", "Error while getting mixer state");
    }
}

void exit_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    outRootNode->Add("error", Jzon::null);
    should_stop = true;
}
