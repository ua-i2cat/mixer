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

class statManager {
    private:
        int mix_delay;
        int input_max_delay;
        int output_max_delay;
        int lost_input_frames_counter;
        int lost_output_frames_counter;
        int total_input_frames_counter;
        int total_output_frames_counter;
        
        int avg_delay;
        int output_frame_rate;
        int mix_avg_delay;
        int input_delay;
        int output_delay;
        int lost_input_frames;
        int total_input_frames;
        int lost_output_frames;
        int total_output_frames;
        int lost_input_frames_percent;
        int lost_output_frames_percent;

        int counter;
        map<string,int> stats_map;
        map<uint32_t, uint32_t> seqno_map;
        map<uint32_t, uint32_t>::iterator seqno_it;

        void update_stats();

    public:
        void update_mix_stat(uint32_t delay);
        void update_input_stat(uint32_t stream_id, uint32_t delay, uint32_t seq_number);
        void update_output_stat(uint32_t delay, bool frame_lost);
        map<string, int>* get_stats();

};

#endif /* STAT_MANAGER_H_*/