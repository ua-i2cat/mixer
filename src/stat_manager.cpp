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

void statManager::update_input_stat(uint32_t stream_id, uint32_t delay, uint32_t seq_number)
{
    if (delay > input_max_delay){
        input_max_delay = delay;
    }

    if (seq_number == seqno_map[stream_id]){
        return;
    }

    lost_input_frames_counter += seq_number - seqno_map[stream_id] - 1;
    seqno_map[stream_id] = seq_number;
}

void statManager::update_output_stat(uint32_t delay, bool frame_lost)
{
    if (delay > output_max_delay){
        output_max_delay = delay;
    }

    if (frame_lost){
        lost_output_frames_counter++;
    }

    total_output_frames_counter++;
}

map<string,int>* statManager::get_stats()
{
    return &stats_map;
}

void statManager::update_stats()
{
    mix_avg_delay = mix_delay/counter;
    input_delay = input_max_delay;
    output_delay = output_max_delay;
    avg_delay = mix_avg_delay + input_delay + output_delay;
    output_frame_rate = 1000000/mix_avg_delay; //NOTE: delay is in us
    mix_delay = 0;
    counter = 0;
    input_max_delay = 0;
    output_max_delay = 0;
    total_input_frames = 0;

    lost_input_frames = lost_input_frames_counter;
    lost_output_frames = lost_output_frames_counter;

    for (seqno_it = seqno_map.begin(); seqno_it != seqno_map.end(); seqno_it++){
        total_input_frames += seqno_it->second +1;
    }

    total_output_frames = total_output_frames_counter;
    lost_input_frames_percent = (lost_input_frames/total_input_frames)*100;
    lost_output_frames_percent = (lost_output_frames/total_output_frames)*100;

    stats_map["avg_delay"] = avg_delay;
    stats_map["mix_avg_delay"] = mix_avg_delay;
    stats_map["output_frame_rate"] = output_frame_rate;
    stats_map["input_delay"] = input_delay;
    stats_map["output_delay"] = output_delay;
    stats_map["lost_input_frames"] = lost_input_frames;
    stats_map["lost_output_frames"] = lost_output_frames;
    stats_map["total_input_frames"] = total_input_frames;
    stats_map["total_output_frames"] = total_output_frames;
    stats_map["lost_input_frames_percent"] = lost_input_frames_percent;
    stats_map["lost_output_frames_percent"] = lost_output_frames_percent;
}

