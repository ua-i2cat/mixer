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

using namespace std;


class Event {

        int delay;
        int socket;
        int timestamp;
        Jzon::Object input_root_node, output_root_node;
        Jzon::Writer writer;

    public:
        
        bool operator<(const Event& e) const;
        Event(Jzon::Object rNode, int ts, int s);
        void send_and_close() const;
        Jzon::Object get_input_root_node() const;
        Jzon::Object get_output_root_node() const;
        int get_timestamp() const;

};

#endif /* EVENT_H_ */