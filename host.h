#ifndef HOST_H
#define HOST_H

#include <setjmp.h>
#include "qtypes.h"
#include "qdefs.h"
#include "cmd.h"

typedef enum host_state_e
{
	host_shutdown,
	host_init,
	host_loading,
	host_active
} host_state_t;

typedef struct host_s
{
	jmp_buf abortframe;
	int state;
	int framecount; // incremented every frame, never reset (checked by Host_Error and Host_SaveConfig_f)
	double realtime; // the accumulated mainloop time since application started (with filtering), without any slowmo or clamping
	double dirtytime; // the main loop wall time for this frame, equal to Sys_DirtyTime() at the start of this host frame
	double sleeptime; // time spent sleeping overall
	qbool restless; // don't sleep
	qbool paused; // global paused state, pauses both client and server
	cmd_buf_t *cbuf;

	struct
	{
		void (*ConnectLocal)(void);
	} hook;
} host_t;

extern host_t host;

void Host_InitCommands(void);
void Host_Main(void);
double Host_Frame(double time);
void Host_Shutdown(void);
void Host_StartVideo(void);
void Host_Error(const char *error, ...) DP_FUNC_PRINTF(1) DP_FUNC_NORETURN;
void Host_NoOperation_f(cmd_state_t *cmd);
void Host_LockSession(void);
void Host_UnlockSession(void);
void Host_AbortCurrentFrame(void);

#endif
