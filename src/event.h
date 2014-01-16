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

/*! Event class defines a Mixer Event. It basically contains a reference to a Mixer class method and the action delay. */ 

class Mixer;

class Event {

        int delay;
        int socket;
        uint32_t timestamp;
        Jzon::Object output_root_node;
        Jzon::Object* input_root_node;
        void(Mixer::*function)(Jzon::Object*, Jzon::Object*);

    public:
        
        /**
        * Class constructor
        * @param fun Mixer method reference associated to the event
        * @param params JSON object containing Mixer function parameters
        * @param ts Timestamp of the Event, which identifies when it can be executed
        * @param s A reference to the socket structure in order to send the method response and close the connection
        */
        Event(void(Mixer::*fun)(Jzon::Object*, Jzon::Object*), Jzon::Object params, int ts, int s);

        /**
        * Redefinition of < operator regarding event queue order
        */
        bool operator<(const Event& e) const;
        
        /**
        * Execute the Mixer associated mehotd
        * @param m Pointer to the Mixer object which will execute the method
        */
        void exec_func(Mixer *m);

        /**
        * Format the response as JSON, sends it to the Event request origin and close the connection
        */
        void send_and_close();
        
        Jzon::Object get_input_root_node();
        Jzon::Object get_output_root_node();
        uint32_t get_timestamp();

};

#endif /* EVENT_H_ */