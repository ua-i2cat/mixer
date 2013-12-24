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

#include "event.h"
#include <iostream>

bool Event::operator<(const Event& e) const
{
    return timestamp > e.timestamp;
}

Event::Event(void(Mixer::*fun)(Jzon::Object*, Jzon::Object*), Jzon::Object params, int ts, int s)
{
    function = fun;
    input_root_node = new Jzon::Object(params);
    timestamp = ts;
    socket = s;
}

void Event::exec_func(Mixer *m)
{
    ((m)->*(this->function))(input_root_node, &output_root_node);
}

void Event::send_and_close() 
{
    Jzon::Writer writer(output_root_node, Jzon::NoFormat);
    writer.Write();
    string result = writer.GetResult();
    const char* res = result.c_str();
    int n = write(socket,res,result.size());
    close(socket);
}

Jzon::Object Event::get_input_root_node() 
{
    return *input_root_node;
}

Jzon::Object Event::get_output_root_node() 
{
    return output_root_node;
}

int Event::get_timestamp() 
{
    return timestamp;
}
