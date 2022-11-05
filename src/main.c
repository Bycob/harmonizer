#include <signal.h>
#include <stdio.h>

#include <jack/jack.h>

#include "harmonizer.h"
#include "jack_backend.h"

#define HANDLE_ERR(Line)                                                       \
    if (Line != 0)                                                             \
    exit(1)

jack_backend_t _jack;

static void signal_handler(int sig) {
    jack_client_close(_jack.client);
    fprintf(stderr, "signal received, exiting ...\n");
    exit(0);
}

int main(int argc, char **argv) {
    HANDLE_ERR(init_jack(&_jack, "client", NULL));

    /* tell the JACK server to call `process()' whenever
       there is work to be done.
    */

    precompute();
    _harmonizer_data.jack = &_jack;
    jack_set_process_callback(_jack.client, harmonizer_process, 0);

    HANDLE_ERR(init_io(&_jack));
    HANDLE_ERR(start_jack(&_jack));

    /* install a signal handler to properly quits jack client */
#ifdef WIN32
    signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
#else
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
#endif

    /* keep running until the transport stops */
    while (1) {
#ifdef WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    jack_client_close(_jack.client);
    exit(0);
}
