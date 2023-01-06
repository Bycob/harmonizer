#ifndef HARMONIZER_MIDI_H
#define HARMONIZER_MIDI_H

#include <stdbool.h>

#include "harmonizer_dsp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

} harmonizer_midi_params_t;

int init_midi(harmonizer_midi_params_t *params);

/** Call in the main thread to collect all midi events */
int poll_midi_events();

/** Get last midi event and remove it from the stack. Returns false if no
 * events. Thread safe function. */
int pop_midi_event(harmonizer_midi_event_t *evt);

int destroy_midi();

#ifdef __cplusplus
}
#endif

#endif
