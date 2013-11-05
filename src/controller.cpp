/*
 * controller.cpp
 *
 *  Created on: Jul 22, 2013
 *      Author: palau
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
void modify_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void modify_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);
void enable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void disable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void add_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);
void remove_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_streams(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_destinations(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);
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
mixer *m;

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
    m = mixer::get_instance();
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
    commands["modify_stream"] = modify_stream;
    commands["modify_layout"] = modify_layout;
    commands["add_destination"] = add_destination;
    commands["enable_stream"] = enable_stream;
    commands["disable_stream"] = disable_stream;
    commands["remove_destination"] = remove_destination;
    commands["get_streams"] = get_streams;
    commands["get_stream"] = get_stream;
    commands["get_destinations"] = get_destinations;
    commands["get_destination"] = get_destination;
    commands["get_layout"] = get_layout;
    commands["get_state"] = get_state;
    commands["exit_mixer"] = exit_mixer;
}

void start_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    m = mixer::get_instance();
    if (m->get_state() == 1){
        outRootNode->Add("error", "Mixer is already running");   
    }else {
        int width = rootNode.Get("params").Get("width").ToInt();
        int height = rootNode.Get("params").Get("height").ToInt();
        int max_streams = rootNode.Get("params").Get("max_streams").ToInt();
        int in_port = rootNode.Get("params").Get("input_port").ToInt();
        int out_port = 56;
        printf("m->init(%d, %d, %d, %d, %d);\nm->exec()\n", 
        width, height, max_streams, in_port, out_port);
        m->init(width, height, max_streams, in_port, out_port); 
        m->exec();
        m->set_state(1);
        outRootNode->Add("error", Jzon::null);      
    }
}

void stop_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running. Cannot stop it");
    }else {
        m->stop();
        m->set_state(0);
        outRootNode->Add("error", Jzon::null);
    } 
}

void add_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        int width = rootNode.Get("params").Get("width").ToInt();
        int height = rootNode.Get("params").Get("height").ToInt();
        int new_w = rootNode.Get("params").Get("new_w").ToInt();
        int new_h = rootNode.Get("params").Get("new_h").ToInt();
        int x = rootNode.Get("params").Get("x").ToInt();
        int y = rootNode.Get("params").Get("y").ToInt();
        int layer = rootNode.Get("params").Get("layer").ToInt();
        if (m->add_source() == -1){
            outRootNode->Add("error", "errore");
        }else {
            outRootNode->Add("error", Jzon::null);
            printf("m->add_source(%d, %d, H264)\n", width, height);
        }
    }
}

void remove_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        int id = rootNode.Get("params").Get("id").ToInt();
        if (m->remove_source(id) == -1){
            outRootNode->Add("error", "errore");
        }else {
            outRootNode->Add("error", Jzon::null);
            printf("m->remove_source(%d)\n", id);
        }
    }
}

void modify_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        int id = rootNode.Get("params").Get("id").ToInt();
        int width = rootNode.Get("params").Get("width").ToInt();
        int height = rootNode.Get("params").Get("height").ToInt();
        int x = rootNode.Get("params").Get("x").ToInt();
        int y = rootNode.Get("params").Get("y").ToInt();
        int layer = rootNode.Get("params").Get("layer").ToInt();
        bool keep_aspect_ratio = rootNode.Get("params").Get("keep_aspect_ratio").ToBool();
        if (m->modify_stream(id, width, height, x, y, layer, keep_aspect_ratio) == -1){
            outRootNode->Add("error", "errore");
        }else {
            outRootNode->Add("error", Jzon::null);
            printf("m->modify_stream(%d, %d, %d, %d, %d, %d, %d);\n", 
                id, width, height, x, y, layer, keep_aspect_ratio);
        }
    }
}

void modify_layout(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        int width = rootNode.Get("params").Get("width").ToInt();
        int height = rootNode.Get("params").Get("height").ToInt();
        bool resize_streams = rootNode.Get("params").Get("resize_streams").ToBool();
        if (m->resize_output(width, height, resize_streams) == -1){
            outRootNode->Add("error", "errore");
        }else {
            outRootNode->Add("error", Jzon::null);
            printf("m->resize_output(%d, %d, %d)\n", width, height, resize_streams);
        }
    } 
}

void enable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        int id = rootNode.Get("params").Get("id").ToInt();
        if (check_stream_id(id) == -1){
            outRootNode->Add("error", "Introduced ID doesn't match any mixer stream ID");
        } else {
            if(m->change_stream_state(id, ACTIVE) == -1){
                outRootNode->Add("error", "Error enabling stream");
            } else {
                outRootNode->Add("error", Jzon::null);
                printf("m->set_stream_active(%d, 1)\n", id);
            }
        }
    }
}

void disable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        uint32_t id = rootNode.Get("params").Get("id").ToInt();
        if (check_stream_id(id) == -1){
            outRootNode->Add("error", "Introduced ID doesn't match any mixer stream ID");
        }else {
            if(m->change_stream_state(id, NON_ACTIVE) == -1){
                outRootNode->Add("error", "Error enabling stream");
            }else {
                outRootNode->Add("error", Jzon::null);
                printf("m->set_stream_active(%d, 0)\n", id);
            }
        }
    }
}

void add_destination(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        std::string ip_string = rootNode.Get("params").Get("ip").ToString();
        uint32_t port = rootNode.Get("params").Get("port").ToInt();
        char *ip = new char[ip_string.length() + 1];
        strcpy(ip, ip_string.c_str());
        if (m->add_destination(ip, port) == -1){
            outRootNode->Add("error", "errore");
        }else {
            outRootNode->Add("error", Jzon::null);
            printf("m->add_destination(H264, %s, %d)\n", ip, port);
        }
    } 
}

void remove_destination(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        uint32_t id = rootNode.Get("params").Get("id").ToInt();
        outRootNode->Add("error", Jzon::null);
        if (m->remove_destination(id) == -1){
            outRootNode->Add("error", "errore");
        }else {
            outRootNode->Add("error", Jzon::null);
            printf("m->remove_destination(%d)\n", id);
        }
    } 
}

void get_streams(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    } else{
        uint32_t i;
        Jzon::Array list;
        std::vector<uint32_t> streams_id = m->get_streams_id();
        if(streams_id.empty()){
            outRootNode->Add("streams", list);
        }else {
            for (i=0; i<streams_id.size(); i++){
                Jzon::Object stream;
                map<string,uint32_t> stream_map;
                m->get_stream_info(stream_map, streams_id[i]);
                stream.Add("id", (int)stream_map["id"]);
                stream.Add("orig_width", (int)stream_map["orig_width"]);
                stream.Add("orig_height", (int)stream_map["orig_height"]);
                stream.Add("width", (int)stream_map["width"]);
                stream.Add("height", (int)stream_map["height"]);
                stream.Add("x", (int)stream_map["x"]);
                stream.Add("y", (int)stream_map["y"]);
                stream.Add("layer", (int)stream_map["layer"]);
                stream.Add("active", (int)stream_map["active"]);
                list.Add(stream);
            }
            outRootNode->Add("streams", list);
        }
    }
}

void get_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    }else {
        int id = rootNode.Get("params").Get("id").ToInt();
        if (check_stream_id(id) == -1){
    	   outRootNode->Add("error", "Introduced ID doesn't match any mixer stream ID");
        }else {
    	   std::map<std::string, uint32_t> stream_map;
    	   m->get_stream_info(stream_map, id);
    	   outRootNode->Add("id", (int)stream_map["id"]);
    	   outRootNode->Add("orig_width", (int)stream_map["orig_width"]);
    	   outRootNode->Add("orig_height", (int)stream_map["orig_height"]);
    	   outRootNode->Add("width", (int)stream_map["width"]);
    	   outRootNode->Add("height", (int)stream_map["height"]);
    	   outRootNode->Add("x", (int)stream_map["x"]);
    	   outRootNode->Add("y", (int)stream_map["y"]);
    	   outRootNode->Add("layer", (int)stream_map["layer"]);
    	   outRootNode->Add("active", (int)stream_map["active"]);
        }
    }
}

void get_destinations(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    }else {
        Jzon::Array list;
        map<uint32_t, mixer::Dst> dst_map = m->get_destinations();
        std::map<uint32_t,mixer::Dst>::iterator it;
        for (it=dst_map.begin(); it!=dst_map.end(); it++){
            Jzon::Object dst;
            string ip;
            int port;
            m->get_destination(it->first, ip, &port);
            dst.Add("id", (int)it->first);
            dst.Add("ip", ip);
            dst.Add("port", port);
            list.Add(dst);
        }
        outRootNode->Add("destinations", list);
    }
}

void get_destination(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    }else {
        int id = rootNode.Get("params").Get("id").ToInt();
        int port;
        string ip;
        if (m->get_destination(id, ip, &port) == -1){
            outRootNode->Add("error", "Destination ID not found");
        }else {
            outRootNode->Add("id", id);
            outRootNode->Add("ip", ip);
            outRootNode->Add("port", port);
        }
    }
}

void get_layout(Jzon::Object rootNode, Jzon::Object *outRootNode){
    if (m->get_state() == 0){
        outRootNode->Add("error", "Mixer is not running!");
    }else {
        int width, height;
        if (m->get_layout_size(&width, &height) == 0){
            outRootNode->Add("width", width);
            outRootNode->Add("height", height);
        }else {
            outRootNode->Add("error", "Error while getting layout size");
        }
    }
}

void get_state(Jzon::Object rootNode, Jzon::Object *outRootNode){
    uint8_t state = m->get_state();
    if (state == 1 || state == 0){
        outRootNode->Add("state", state);
    }else{
        outRootNode->Add("error", "Error while getting mixer state");
    }
}

int check_stream_id(uint32_t id){
	int i;
	std::vector<uint32_t> streams_id = m->get_streams_id();
	for (i=0; i<streams_id.size(); i++){
		if (streams_id[i] == id){
			return 0;
		}
	}
	return -1;
}

void exit_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    outRootNode->Add("error", Jzon::null);
    should_stop = true;
}