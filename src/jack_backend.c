#include "jack_backend.h"

void jack_shutdown(void *arg) {
    fprintf(stderr, "Jack stopped, please abort program...");
}

int init_jack(jack_backend_t *jack, char *client_name, char *server_name) {
    int i;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    if (server_name != 0) /* server name specified? */
    {
        options |= JackServerName;
    }

    /* open a client connection to the JACK server */
    jack->client = jack_client_open(client_name, options, &status, server_name);
    if (jack->client == NULL) {
        fprintf(stderr,
                "jack_client_open() failed, "
                "status = 0x%2.0x\n",
                status);
        if (status & JackServerFailed) {
            fprintf(stderr, "Unable to connect to JACK server\n");
        }
        return 1;
    }
    if (status & JackServerStarted) {
        fprintf(stderr, "JACK server started\n");
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(jack->client);
        fprintf(stderr, "unique name `%s' assigned\n", client_name);
    }

    /* tell the JACK server to call `jack_shutdown()' if
       it ever shuts down, either entirely, or if it
       just decides to stop calling us.
    */

    jack_on_shutdown(jack->client, jack_shutdown, 0);
    return 0;
}

int init_io(jack_backend_t *jack) {
    /* create two ports pairs*/
    jack->input_ports = (jack_port_t **)calloc(2, sizeof(jack_port_t *));
    jack->output_ports = (jack_port_t **)calloc(2, sizeof(jack_port_t *));

    int i;
    char port_name[16];
    for (i = 0; i < 2; i++) {
        sprintf(port_name, "input_%d", i + 1);
        jack->input_ports[i] =
            jack_port_register(jack->client, port_name, JACK_DEFAULT_AUDIO_TYPE,
                               JackPortIsInput, 0);
        sprintf(port_name, "output_%d", i + 1);
        jack->output_ports[i] =
            jack_port_register(jack->client, port_name, JACK_DEFAULT_AUDIO_TYPE,
                               JackPortIsOutput, 0);
        if ((jack->input_ports[i] == NULL) || (jack->output_ports[i] == NULL)) {
            fprintf(stderr, "no more JACK ports available\n");
            return 1;
        }
    }
    return 0;
}

int start_jack(jack_backend_t *jack) {
    /* Tell the JACK server that we are ready to roll.  Our
     * process() callback will start running now. */
    const char **ports;

    if (jack_activate(jack->client)) {
        fprintf(stderr, "cannot activate client");
        return 1;
    }

    /* Connect the ports.  You can't do this before the client is
     * activated, because we can't make connections to clients
     * that aren't running.  Note the confusing (but necessary)
     * orientation of the driver backend ports: playback ports are
     * "input" to the backend, and capture ports are "output" from
     * it.
     */

    int i;
    ports = jack_get_ports(jack->client, NULL, NULL,
                           JackPortIsPhysical | JackPortIsOutput);
    if (ports == NULL) {
        fprintf(stderr, "no physical capture ports\n");
        return 1;
    }

    jack->stereo_input = true;
    for (i = 0; i < 2; i++) {
        fprintf(stderr, "port name = %s, port=%s\n",
                jack_port_name(jack->input_ports[i]), ports[i]);
        if (jack_connect(jack->client, ports[i],
                         jack_port_name(jack->input_ports[i]))) {
            jack->stereo_input = false;
            fprintf(stderr, "cannot connect input ports\n");
        }
    }

    free(ports);

    ports = jack_get_ports(jack->client, NULL, NULL,
                           JackPortIsPhysical | JackPortIsInput);
    if (ports == NULL) {
        fprintf(stderr, "no physical playback ports\n");
        return 1;
    }

    for (i = 0; i < 2; i++) {
        fprintf(stderr, "port name = %s, port=%s\n",
                jack_port_name(jack->output_ports[i]), ports[i]);
        if (jack_connect(jack->client, jack_port_name(jack->output_ports[i]),
                         ports[i]))
            fprintf(stderr, "cannot connect input ports\n");
    }

    free(ports);
    return 0;
}
