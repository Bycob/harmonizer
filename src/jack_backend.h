
#ifndef JACK_BACKEND_H
#define JACK_BACKEND_H

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>

typedef struct {
    jack_port_t **input_ports;
    jack_port_t **output_ports;
    jack_client_t *client;

    bool stereo_input;
} jack_backend_t;

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown(void *arg);

int init_jack(jack_backend_t *jack, char *client_name, char *server_name);

int init_io(jack_backend_t *jack);

int start_jack(jack_backend_t *jack);

#endif // JACK_BACKEND_H
