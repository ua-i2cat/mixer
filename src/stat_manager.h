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

#ifndef STAT_MANAGER_H_
#define STAT_MANAGER_H_

#include <map>
#include <stdint.h>
#include <string>

#define STATS_FRAME_WINDOW 100

using namespace std;

class streamStats {
    public:
        streamStats(uint32_t str_id);
        int get_delay();
        int get_fps();
        int get_bitrate();
        int get_lost_coded_frames();
        int get_lost_frames();
        int get_total_frames();
        int get_lost_frames_percent();
        void set_delay(int _delay);
        void set_fps(int _fps);
        void set_bitrate(int _bitrate);
        void set_lost_coded_frames(int _lost_coded_frames);
        void set_lost_frames(int _lost_frames);
        void set_total_frames(int _total_frames);
        void set_lost_frames_percent(int _lost_frames_percent);


    private:
        uint32_t id;
        int delay;
        int fps;
        int bitrate;
        int lost_coded_frames;
        int lost_frames;
        int total_frames;
        int lost_frames_percent;
};

class statManager {
    private:
        int mix_delay;
        int mixing_avg_delay;
        int input_max_delay;
        int output_max_delay;
        int total_delay;
        int output_frame_rate;
        int lost_input_frames;
        int total_input_frames;
        int lost_output_frames;
        int total_output_frames;
        int lost_input_frames_percent;
        int lost_output_frames_percent;

        int counter;
        map<string,int> stats_map;
        map<uint32_t, streamStats*> input_str_map;
        map<uint32_t, streamStats*>::iterator input_str_it;
        map<uint32_t, streamStats*> output_str_map;
        map<uint32_t, streamStats*>::iterator output_str_it;

        void update_stats();

    public:
        statManager();
        void update_mix_stat(uint32_t delay);
        void update_input_stat(uint32_t id, float decode_delay, float resize_delay, float fps, 
                               uint32_t seq_number, uint32_t lost_coded_frames,  uint32_t bitrate);
        void update_output_stat(uint32_t id, float resize_encoding_delay, float tx_delay, 
                                float fps, uint32_t seq_number, uint32_t lost_coded_frames);
        void add_input_stream(uint32_t id);
        void remove_input_stream(uint32_t id);
        void add_output_stream(uint32_t id);
        void remove_output_stream(uint32_t id);
        void get_stats(map<uint32_t,streamStats*> &input_stats, map<uint32_t,streamStats*> &output_stats);
        void output_frame_lost(uint32_t id);


};

#endif /* STAT_MANAGER_H_*/