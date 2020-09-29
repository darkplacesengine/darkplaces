#ifndef SV_DEMO_H
#define SV_DEMO_H

#include "qtypes.h"
#include "common.h"
typedef struct client_s client_t;

void SV_StartDemoRecording(client_t *client, const char *filename, int forcetrack);
void SV_WriteDemoMessage(client_t *client, sizebuf_t *sendbuffer, qbool clienttoserver);
void SV_StopDemoRecording(client_t *client);
void SV_WriteNetnameIntoDemo(client_t *client);

#endif
