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

using namespace std;


void statManager::update_mix_stat(uint32_t delay)
{
    mix_delay += delay;
    counter++;

    if (counter == STATS_FRAME_WINDOW - 1){  
        update_stats();
    }

}

void statManager::update_input_stat(uint32_t id, uint32_t delay, uint32_t seq_number, uint32_t lost_coded_frames, uint32_t fps, uint32_t bitrate)
{
    if (input_str_map.count(id) < 1){
        return;
    }

    streamStats *stats = input_str_map[id];

    stats->set_delay(delay);
    stats->set_fps(fps);
    stats->set_bitrate(bitrate);
    stats->set_lost_coded_frames(lost_coded_frames);
    stats->set_lost_frames(seq_number - stats->get_total_frames() - 1);
    stats->set_total_frames(seq_number);
    stats->set_lost_frames_percent(((stats->get_lost_frames())*100)/stats->get_total_frames()); 
}

void statManager::update_output_stat(uint32_t delay, bool frame_lost)
{
    output_max_delay = delay;

    if (frame_lost){
        lost_output_frames_counter++;
    }

    total_output_frames_counter++;
}

void statManager::get_stats(map<string,int>* stats, map<uint32_t,streamStats*> &input_stats)
{
    stats = &stats_map;
    input_stats = input_str_map;
    cout << "Manager: " << input_stats.size() << endl;
}

void statManager::update_stats()
{
    // mix_avg_delay = mix_delay/counter;
    // input_delay = input_max_delay;
    // output_delay = output_max_delay;
    // avg_delay = mix_avg_delay + input_delay + output_delay;
    // output_frame_rate = 1000000/mix_avg_delay; //NOTE: delay is in us
    // mix_delay = 0;
    // counter = 0;
    // input_max_delay = 0;
    // output_max_delay = 0;
    // total_input_frames = 0;

    // lost_input_frames = lost_input_frames_counter;
    // lost_output_frames = lost_output_frames_counter;

    // for (seqno_it = seqno_map.begin(); seqno_it != seqno_map.end(); seqno_it++){
    //     total_input_frames += seqno_it->second +1;
    // }

    // total_output_frames = total_output_frames_counter;
    // lost_input_frames_percent = (lost_input_frames/total_input_frames)*100;
    // lost_output_frames_percent = (lost_output_frames/total_output_frames)*100;

    stats_map["avg_delay"] = 0;
    stats_map["mix_avg_delay"] = 0;
    stats_map["output_frame_rate"] = 0;
    stats_map["input_delay"] = 0;
    stats_map["output_delay"] = 0;
    stats_map["lost_input_frames"] = 0;
    stats_map["lost_output_frames"] = 0;
    stats_map["total_input_frames"] = 0;
    stats_map["total_output_frames"] = 0;
    stats_map["lost_input_frames_percent"] = 0;
    stats_map["lost_output_frames_percent"] = 0;

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

streamStats::streamStats(uint32_t str_id)
{
    id = str_id;
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

int streamStats::get_lost_frames()
{
    return lost_frames;
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

void streamStats::set_lost_frames(int _lost_frames)
{
    lost_frames = _lost_frames;
}

void streamStats::set_total_frames(int _total_frames)
{
    total_frames = _total_frames;
}

void streamStats::set_lost_frames_percent(int _lost_frames_percent)
{
    lost_frames_percent = _lost_frames_percent;
}


