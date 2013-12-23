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

#ifndef EVENT_H_
#define EVENT_H_

#include <map>
#include "Jzon.h"
#include "mixer.h"

using namespace std;

class Mixer;

class Event {

        int delay;
        int socket;
        int timestamp;
        Jzon::Object input_root_node, output_root_node;
        void(Mixer::*function)(Jzon::Object, Jzon::Object*);

    public:
        
        bool operator<(const Event& e) const;
        Event(void(Mixer::*fun)(Jzon::Object, Jzon::Object*), Jzon::Object params, int ts, int s);
        void exec_func(Mixer *m);
        void send_and_close();
        Jzon::Object get_input_root_node();
        Jzon::Object get_output_root_node();
        int get_timestamp();

};

#endif /* EVENT_H_ */