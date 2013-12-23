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

void start_mixer(Jzon::Object rootNode);
void stop_mixer(Jzon::Object rootNode);

void add_stream(Jzon::Object rootNode);
void remove_stream(Jzon::Object rootNode);
void add_crop_to_stream(Jzon::Object rootNode);
void modify_crop_from_stream(Jzon::Object rootNode);
void modify_crop_resizing_from_stream(Jzon::Object rootNode);
void remove_crop_from_stream(Jzon::Object rootNode);

void add_crop_to_layout(Jzon::Object rootNode);
void modify_crop_from_layout(Jzon::Object rootNode);
void modify_crop_resizing_from_layout(Jzon::Object rootNode);
void remove_crop_from_layout(Jzon::Object rootNode);

void enable_crop_from_stream(Jzon::Object rootNode);
void disable_crop_from_stream(Jzon::Object rootNode);

void add_destination(Jzon::Object rootNode);
void remove_destination(Jzon::Object rootNode);

void get_streams(Jzon::Object rootNode);
void get_layout(Jzon::Object rootNode);
void get_stats(Jzon::Object rootNode);
void get_layout_size(Jzon::Object rootNode);
void get_state(Jzon::Object rootNode);
void exit_mixer(Jzon::Object rootNode);

void initialize_action_mapping();
int get_socket(int port, int *sock);
int listen_socket(int sock, int *newsock);

std::map<std::string, void(*)(Jzon::Object, Jzon::Object*)> commands;
Jzon::Object rootNode;
Jzon::Parser parser(rootNode);
bool should_stop=false;
Mixer *m;
string action;

void start_mixer(Jzon::Object rootNode, int socket);
void stop_mixer(Jzon::Object rootNode, int socket);
void exit(Jzon::Object rootNode, int socket);


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

    while(!should_stop) {
        if (listen_socket(sockfd, &newsockfd) == 0) {
            bzero(buffer,2048);
            rootNode.Clear();
            n = read(newsockfd, buffer, 2047);
            if (n < 0) {
                error("ERROR reading from socket");
            }

            parser.SetJson(buffer);

            if (parser.Parse()) {
                action = rootNode.Get("action").ToString();
                if (commands.count(action) <= 0){
                    break;
                }

                commands[action](rootNode);
                gettimeofday(&in_time, NULL);
                m->push_event(Event(commands[action], rootNode, in_time.tv_sec*1000000 + in_time.tv_usec, newsockfd));

            }
        }
    }
    close(sockfd);
    return 0; 
 }


void start_mixer(Jzon::Object rootNode){
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

void stop_mixer(Jzon::Object rootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running. Cannot stop it");
        return;
    }
    
    m->stop();
    m->set_state(0);
    outRootNode->Add("error", Jzon::null);
}

void add_stream(Jzon::Object rootNode){
    if (m->get_state() == 0){
        return;
    }

    gettimeofday(&in_time, NULL);
    int ts = in_time.tv_sec*1000000 + in_time.tv_usec + ootNode.Get("delay").ToInt()*1000000
    m->push_event(Event(commands[rootNode.Get("action").ToString()], rootNode.Get("params"), ts, newsockfd));
}

void remove_stream(Jzon::Object rootNode){
    if (m->get_state() == 0){
        return;
    }

    gettimeofday(&in_time, NULL);
    int ts = in_time.tv_sec*1000000 + in_time.tv_usec + ootNode.Get("delay").ToInt()*1000000
    m->push_event(Event(commands[rootNode.Get("action").ToString()], rootNode.Get("params"), ts, newsockfd));

}

void add_crop_to_stream(Jzon::Object rootNode)
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

void modify_crop_from_stream(Jzon::Object rootNode)
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

void modify_crop_resizing_from_stream(Jzon::Object rootNode)
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

void remove_crop_from_stream(Jzon::Object rootNode)
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

void add_crop_to_layout(Jzon::Object rootNode)
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

void modify_crop_from_layout(Jzon::Object rootNode)
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

void modify_crop_resizing_from_layout(Jzon::Object rootNode){
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

void remove_crop_from_layout(Jzon::Object rootNode)
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

void enable_crop_from_stream(Jzon::Object rootNode)
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

void disable_crop_from_stream(Jzon::Object rootNode)
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

void add_destination(Jzon::Object rootNode)
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
    
    int id = m->add_destination(ip, port, stream_id);

    if (id == FALSE){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("id", id);
    }
}

void remove_destination(Jzon::Object rootNode)
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

void get_streams(Jzon::Object rootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }
    pthread_rwlock_rdlock(m->get_task_lock());

    Jzon::Array stream_list;
    if(m->get_layout()->get_streams()->empty()){
        outRootNode->Add("input_streams", stream_list);
        pthread_rwlock_unlock(m->get_task_lock());
        return;
    }

    std::map<uint32_t, Stream*>* stream_map;
    std::map<uint32_t, Crop*>* crop_map;
    std::map<uint32_t, Stream*>::iterator stream_it;
    std::map<uint32_t, Crop*>::iterator crop_it;

    stream_map = m->get_layout()->get_streams();
    
    for (stream_it = stream_map->begin(); stream_it != stream_map->end(); stream_it++){
        Jzon::Object stream;
        Jzon::Array crop_list;
        stream.Add("id", (int)stream_it->second->get_id());
        stream.Add("width", (int)stream_it->second->get_width());
        stream.Add("height", (int)stream_it->second->get_height());
        crop_map = stream_it->second->get_crops();
        for (crop_it = crop_map->begin(); crop_it != crop_map->end(); crop_it++){
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
    pthread_rwlock_unlock(m->get_task_lock());
}

void get_layout(Jzon::Object rootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }
    pthread_rwlock_rdlock(m->get_task_lock());

    Jzon::Array crop_list;
    std::map<uint32_t, Crop*>::iterator crop_it;
    std::map<uint32_t, Crop*> *crp = m->get_layout()->get_out_stream()->get_crops();
    std::vector<Mixer::Dst>::iterator dst_it;
    std::vector<Mixer::Dst> dst;

    Jzon::Object stream;
    stream.Add("id", (int)m->get_layout()->get_out_stream()->get_id());
    stream.Add("width", (int)m->get_layout()->get_out_stream()->get_width());
    stream.Add("height", (int)m->get_layout()->get_out_stream()->get_height());
    for (crop_it = crp->begin(); crop_it != crp->end(); crop_it++){
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
    pthread_rwlock_unlock(m->get_task_lock());
}

void get_layout_size(Jzon::Object rootNode)
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

void get_stats(Jzon::Object rootNode)
{
#ifdef STATS
    pthread_rwlock_rdlock(m->get_task_lock());
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
        return;
    }

    map<uint32_t,streamStats*> input_stats; 
    map<uint32_t,streamStats*> output_stats; 
    m->get_stats(input_stats, output_stats);
    map<uint32_t,streamStats*>::iterator str_it; 

    Jzon::Array input_list;
    for (str_it = input_stats.begin(); str_it != input_stats.end(); str_it++){
        Jzon::Object str;
        str.Add("id", (int)str_it->first);
        str.Add("delay", (int)str_it->second->get_delay());
        str.Add("fps", (int)str_it->second->get_fps());
        str.Add("bitrate", (int)str_it->second->get_bitrate());
        str.Add("lost_coded_frames", (int)str_it->second->get_lost_coded_frames());
        str.Add("lost_frames", (int)str_it->second->get_lost_frames());
        str.Add("total_frames", (int)str_it->second->get_total_frames());
        str.Add("lost_frames_percent", (int)str_it->second->get_lost_frames_percent());
        input_list.Add(str);
    }
    outRootNode->Add("input_streams", input_list);

    Jzon::Array output_list;
    for (str_it = output_stats.begin(); str_it != output_stats.end(); str_it++){
        Jzon::Object str;
        str.Add("id", (int)str_it->first);
        str.Add("delay", (int)str_it->second->get_delay());
        str.Add("fps", (int)str_it->second->get_fps());
        str.Add("bitrate", (int)str_it->second->get_bitrate());
        str.Add("lost_coded_frames", (int)str_it->second->get_lost_coded_frames());
        str.Add("lost_frames", (int)str_it->second->get_lost_frames());
        str.Add("total_frames", (int)str_it->second->get_total_frames());
        str.Add("lost_frames_percent", (int)str_it->second->get_lost_frames_percent());
        output_list.Add(str);
    }
    outRootNode->Add("output_streams", output_list);

    pthread_rwlock_unlock(m->get_task_lock());
#endif
}


void get_state(Jzon::Object rootNode){
    uint8_t state = m->get_state();
    if (state == 1 || state == 0){
        outRootNode->Add("state", state);
    }else{
        outRootNode->Add("error", "Error while getting mixer state");
    }
}

void exit_mixer(Jzon::Object rootNode){
    outRootNode->Add("error", Jzon::null);
    should_stop = true;
}