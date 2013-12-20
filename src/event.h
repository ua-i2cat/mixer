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

using namespace std;


class Event {

        int delay;
        int socket;
        int timestamp;
        map<string, void(*)(Jzon::Object, Jzon::Object*)>* mixer_commands;
        Jzon::Object input_root_node, output_root_node;
        Jzon::Writer writer;(output_root_node, Jzon::NoFormat);

    public:
        
        bool operator<(const Event& e) const
        {
            return timestamp > e.timestamp;
        }

        Event(Jzon::Object rNode, int ts, int s)
        {
            input_root_node = rNode;
            delay = rNode.Get("delay").ToInt();
            timestamp = ts + delay;
            socket = s;
        }

        void exec_command()
        {
            *commands[rootNode.Get("action").ToString()](input_root_node, &output_root_node);
            writer.Write();
            result = writer.GetResult();
            res = result.c_str();
            n = write(newsockfd,res,result.size());
            close(socket);
        }
};

#endif /* EVENT_H_ */