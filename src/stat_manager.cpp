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
 */     

#include "stat_manager.h"
#include <iostream>
#include <iomanip>
#include <stdlib.h>

using namespace std;

statManager::statManager()
{
    cout << " Delay  " << "In_delay  " << "Mix_delay  " << "Out_delay  " 
    << "%InDec_losses  " << "%InCod_losses  " << "%OutDec_losses  " 
    << "%OutCod_losses  " << "Out_fps" << endl;
}

void statManager::update_mix_stat(uint32_t delay)
{
    mix_delay += delay;
    counter++;

    if (counter == STATS_FRAME_WINDOW - 1){  
        update_stats();
    }

}

void statManager::update_input_stat(uint32_t id, float decode_delay, float resize_delay, float fps, 
                                    uint32_t seq_number, uint32_t lost_coded_frames, uint32_t bitrate)
{
    if (input_str_map.count(id) < 1){
        return;
    }

    streamStats *stats = input_str_map[id];

    stats->set_delay(decode_delay + resize_delay);
    stats->set_fps(fps);
    stats->set_bitrate(bitrate);
    stats->set_lost_coded_frames(lost_coded_frames);
    stats->set_lost_decoded_frames(stats->get_lost_decoded_frames() + seq_number - stats->get_total_frames() - 1);
    stats->set_total_frames(seq_number);
    stats->set_lost_frames_percent(((stats->get_lost_decoded_frames() + lost_coded_frames)*100)/(stats->get_total_frames() + lost_coded_frames)); 
}

void statManager::update_output_stat(uint32_t id, float resize_encoding_delay, float tx_delay, float fps,
                                     uint32_t seq_number, uint32_t lost_coded_frames)
{
    if (output_str_map.count(id) < 1){
        return;
    }

    streamStats *stats = output_str_map[id];

    stats->set_delay(resize_encoding_delay + tx_delay);
    stats->set_fps(fps);
    stats->set_bitrate(0);
    stats->set_lost_coded_frames(lost_coded_frames);
    stats->set_total_frames(seq_number);
    stats->set_lost_frames_percent(((stats->get_lost_decoded_frames() + lost_coded_frames)*100)/(stats->get_total_frames())); 
}

void statManager::get_stats(map<uint32_t,streamStats*> &input_stats, map<uint32_t,streamStats*> &output_stats)
{
    input_stats = input_str_map;
    output_stats = output_str_map;
}  

void statManager::update_stats()
{
    mixing_avg_delay = mix_delay/counter;
    
    mix_delay = 0;
    counter = 0;
    lost_coded_input_frames = 0;
    lost_decoded_input_frames = 0;
    total_input_frames = 0;
    lost_coded_output_frames = 0;
    lost_decoded_output_frames = 0;
    total_output_frames = 0;
    input_max_delay = 0;
    output_max_delay = 0;
    output_frame_rate = 1000; 

    for (input_str_it = input_str_map.begin(); input_str_it != input_str_map.end(); input_str_it++){
        total_input_frames += input_str_it->second->get_total_frames() + input_str_it->second->get_lost_coded_frames();
        lost_coded_input_frames += input_str_it->second->get_lost_coded_frames();
        lost_decoded_input_frames += input_str_it->second->get_lost_decoded_frames();

        if (input_str_it->second->get_delay() > input_max_delay){
            input_max_delay = input_str_it->second->get_delay();
        }
    }

    for (output_str_it = output_str_map.begin(); output_str_it != output_str_map.end(); output_str_it++){
        total_output_frames += output_str_it->second->get_total_frames();
        lost_coded_output_frames += output_str_it->second->get_lost_coded_frames();
        lost_decoded_output_frames += output_str_it->second->get_lost_decoded_frames();

        if (output_str_it->second->get_delay() > output_max_delay){
            output_max_delay = output_str_it->second->get_delay();
        }

        if (output_frame_rate > output_str_it->second->get_fps()){
            output_frame_rate = output_str_it->second->get_fps();
        }
    }

    total_delay = input_max_delay + mixing_avg_delay + output_max_delay;

    cout << "\r" << setw(6) << total_delay << "  " << setw(8) << input_max_delay << "  " << 
    setw(9) << mixing_avg_delay << "  " << setw(9) << output_max_delay << "  " << setw(13) << 
    (lost_decoded_input_frames*100)/total_input_frames << setw(14) << " " << 
    (lost_coded_input_frames*100)/total_input_frames << "  " << setw(14) << 
    (lost_decoded_output_frames*100)/total_output_frames <<  " " << setw(15) << 
    (lost_coded_output_frames*100)/total_output_frames << "  " << setw(7) << 
    output_frame_rate << "           " << flush;
}

void statManager::add_input_stream(uint32_t id)
{
    if (input_str_map.count(id) > 0){
        return;
    }
    streamStats *str_stats = new streamStats(id);
    input_str_map[id] = str_stats;
}
        
void statManager::remove_input_stream(uint32_t id)
{
    if (input_str_map.count(id) < 1){
        return;
    }

    map<uint32_t, streamStats*>::iterator it = input_str_map.find(id);
    delete it->second;
    input_str_map.erase(it);
}

void statManager::add_output_stream(uint32_t id)
{
    if (output_str_map.count(id) > 0){
        return;
    }
    streamStats *str_stats = new streamStats(id);
    output_str_map[id] = str_stats;
}
        
void statManager::remove_output_stream(uint32_t id)
{
    if (output_str_map.count(id) < 1){
        return;
    }

    map<uint32_t, streamStats*>::iterator it = output_str_map.find(id);
    delete it->second;
    output_str_map.erase(it);
}

void statManager::output_frame_lost(uint32_t id)
{
    if (output_str_map.count(id) < 1){
        return;
    }

    streamStats *stats = output_str_map[id];

    stats->set_lost_decoded_frames(stats->get_lost_decoded_frames() + 1);
}

streamStats::streamStats(uint32_t str_id)
{
    id = str_id;
    delay = 0;
    fps = 0;
    bitrate = 0;
    lost_coded_frames = 0;
    lost_decoded_frames = 0;
    total_frames = 0;
    lost_frames_percent = 0;
}

int streamStats::get_delay()
{
    return delay;    
}

int streamStats::get_fps()
{
    return fps;
}

int streamStats::get_bitrate()
{
    return bitrate;
}

int streamStats::get_lost_coded_frames()
{
    return lost_coded_frames;
}

int streamStats::get_lost_decoded_frames()
{
    return lost_decoded_frames;
}

int streamStats::get_total_frames()
{
    return total_frames;
}

int streamStats::get_lost_frames_percent()
{
    return lost_frames_percent;
}

void streamStats::set_delay(int _delay)
{
    delay = _delay;
}

void streamStats::set_fps(int _fps)
{
    fps = _fps;
}

void streamStats::set_bitrate(int _bitrate)
{
    bitrate = _bitrate;
}

void streamStats::set_lost_coded_frames(int _lost_coded_frames)
{
    lost_coded_frames = _lost_coded_frames;
}

void streamStats::set_lost_decoded_frames(int _lost_frames)
{
    lost_decoded_frames = _lost_frames;
}

void streamStats::set_total_frames(int _total_frames)
{
    total_frames = _total_frames;
}

void streamStats::set_lost_frames_percent(int _lost_frames_percent)
{
    lost_frames_percent = _lost_frames_percent;
}


