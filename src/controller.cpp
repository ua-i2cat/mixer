/*
 * controller.cpp
 *
 *  Created on: Jul 22, 2013
 *      Author: palau
 */

#include "mixer.h"

int main(int argc, char *argv[]){

	mixer* m = mixer::get_instance();

	uint32_t layout_width = 1920;
	uint32_t layout_height = 1080;
	uint32_t max_streams = 4;
	uint32_t in_port = 5004;
	uint32_t out_port = 56;

	m->init(layout_width, layout_height, max_streams, in_port, out_port);
	m->exec();

	int option=0;
	uint32_t width=0, height=0, codec=0, id=0, port=0, x=0, y=0, layer=0, keepratio=0, resizestreams=0, framerate=0;
	bool keep_aspect_ratio = false, resize_streams = false, should_stop = false;
	char ip[20];

	while (!should_stop){
		printf("What do you want to do?\n");
		printf("1 - Add a new source\n");
		printf("2 - Remove a source\n");
		printf("3 - Add a new destination\n");
		printf("4 - Remove a destination\n");
		printf("5 - Modify stream parameters\n");
		printf("6 - Resize output layout\n");
		printf("7 - Modify max frame rate\n");
		printf("8 - Stop the mixer and quit\n");

		scanf ("%d",&option);

		switch (option){
		case 1:
			printf("\nYou have chosen to add a new source.\n"
					"Please introduce the following values this way: width height codec\n");
			scanf("%u %u", &width, &height);
			if (m->add_source(width, height, H264) == -1)
				printf ("\nError while adding the new source\n");
			break;

		case 2:
			printf("\nYou have chosen to remove a source.\n"
					"Please introduce the id of the source you want to remove:\n");
			scanf("%u", &id);
			if (m->remove_source(id) == -1)
				printf ("\nError while removing the source\n");
			break;

		case 3:
			printf("\nYou have chosen to add a destination.\n"
					"Please introduce the following values this way: codec ip port\n");
			scanf("%s %u", &ip, &port);
			if (m->add_destination(H264, ip, port) == -1)
				printf ("\nError while adding a new destination\n");
			break;

		case 4:
			printf("\nYou have chosen to remove a destination.\n"
					"Please introduce the id of the destination you want to remove:\n");
			scanf("%u", &id);
			if (m->remove_destination(id) == -1)
				printf ("\nError while removing the destination\n");
			break;

		case 5:
			printf("\nYou have chosen to modify parameters from a stream\n"
					"Please introduce the following values this way: id width height x y layer keep_aspect_ratio\n");
			scanf("%u %u %u %u %u %u %u", &id, &width, &height, &x, &y, &layer, &keepratio);
			if (keepratio == 1){
				keep_aspect_ratio = true;
			} else {
				keep_aspect_ratio = false;
			}

			if (m->modify_stream(id, width, height, x, y, layer, keep_aspect_ratio) == -1)
				printf ("\nError while modifying the stream\n");
			break;

		case 6:
			printf("\nYou have chosen to resize the output layout.\n"
					"Please introduce the following values this way: width height resize_streams\n");
			scanf("%u %u %u", &width, &height, &resize_streams);
			if ( resizestreams == 1){
				resize_streams = true;
			} else {
				resize_streams = false;
			}

			if (m->resize_output(width, height, resize_streams) == -1)
				printf ("\nError while resizing the layout\n");
			break;

		case 7:
			printf("\nYou have chosen to change the maximum frame rate.\n"
					"Please introduce the new frame rate: ");
			scanf("%u", &framerate);

			m->change_max_framerate(framerate);
			break;

		case 8:
			printf("\nYou have chosen to stop the mixer and quit.\n");
			m->stop();
			should_stop = true;
			printf("Mixer stopped correctly. See you later!\n");
			break;

		default:
			printf("Incorrect option. Please introduce a valid option:\n");
			break;
		}
	}
}



