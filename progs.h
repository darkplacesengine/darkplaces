/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef PROGS_H
#define PROGS_H
#include "pr_comp.h"			// defs shared with qcc

typedef struct link_s
{
	int entitynumber;
	struct link_s	*prev, *next;
} link_t;

#define ENTITYGRIDAREAS 16
#define MAX_ENTITYCLUSTERS 16

typedef struct edict_engineprivate_s
{
	// true if this edict is unused
	qboolean free;
	// sv.time when the object was freed (to prevent early reuse which could
	// mess up client interpolation or obscure severe QuakeC bugs)
	float freetime;
	// initially false to prevent projectiles from moving on their first frame
	// (even if they were spawned by an synchronous client think)
	qboolean move;

	// cached cluster links for quick stationary object visibility checking
	vec3_t cullmins, cullmaxs;
	int pvs_numclusters;
	int pvs_clusterlist[MAX_ENTITYCLUSTERS];

	// physics grid areas this edict is linked into
	link_t areagrid[ENTITYGRIDAREAS];
	// since the areagrid can have multiple references to one entity,
	// we should avoid extensive checking on entities already encountered
	int areagridmarknumber;

	// PROTOCOL_QUAKE, PROTOCOL_QUAKEDP, PROTOCOL_NEHAHRAMOVIE, PROTOCOL_QUAKEWORLD
	// baseline values
	entity_state_t baseline;

	// LordHavoc: gross hack to make floating items still work
	int suspendedinairflag;
	// used by PushMove to keep track of where objects were before they were
	// moved, in case they need to be moved back
	vec3_t moved_from;
	vec3_t moved_fromangles;
}
edict_engineprivate_t;

// LordHavoc: in an effort to eliminate time wasted on GetEdictFieldValue...  see pr_edict.c for the functions which use these.
extern int eval_gravity;
extern int eval_button3;
extern int eval_button4;
extern int eval_button5;
extern int eval_button6;
extern int eval_button7;
extern int eval_button8;
extern int eval_button9;
extern int eval_button10;
extern int eval_button11;
extern int eval_button12;
extern int eval_button13;
extern int eval_button14;
extern int eval_button15;
extern int eval_button16;
extern int eval_buttonuse;
extern int eval_buttonchat;
extern int eval_glow_size;
extern int eval_glow_trail;
extern int eval_glow_color;
extern int eval_items2;
extern int eval_scale;
extern int eval_alpha;
extern int eval_renderamt; // HalfLife support
extern int eval_rendermode; // HalfLife support
extern int eval_fullbright;
extern int eval_ammo_shells1;
extern int eval_ammo_nails1;
extern int eval_ammo_lava_nails;
extern int eval_ammo_rockets1;
extern int eval_ammo_multi_rockets;
extern int eval_ammo_cells1;
extern int eval_ammo_plasma;
extern int eval_idealpitch;
extern int eval_pitch_speed;
extern int eval_viewmodelforclient;
extern int eval_nodrawtoclient;
extern int eval_exteriormodeltoclient;
extern int eval_drawonlytoclient;
extern int eval_ping;
extern int eval_movement;
extern int eval_pmodel;
extern int eval_punchvector;
extern int eval_viewzoom;
extern int eval_clientcolors;
extern int eval_tag_entity;
extern int eval_tag_index;
extern int eval_light_lev;
extern int eval_color;
extern int eval_style;
extern int eval_pflags;
extern int eval_cursor_active;
extern int eval_cursor_screen;
extern int eval_cursor_trace_start;
extern int eval_cursor_trace_endpos;
extern int eval_cursor_trace_ent;
extern int eval_colormod;
extern int eval_playermodel;
extern int eval_playerskin;
extern int eval_SendEntity;
extern int eval_Version;
extern int eval_customizeentityforclient;
extern int eval_dphitcontentsmask;
// DRESK - Support for Entity Contents Transition Event
extern int eval_contentstransition;

extern int gval_trace_dpstartcontents;
extern int gval_trace_dphitcontents;
extern int gval_trace_dphitq3surfaceflags;
extern int gval_trace_dphittexturename;



extern mfunction_t *SV_PlayerPhysicsQC;
extern mfunction_t *EndFrameQC;
//KrimZon - SERVER COMMANDS IN QUAKEC
extern mfunction_t *SV_ParseClientCommandQC;

#endif






//////////////////////////////////
#if 0

#include "pr_comp.h"			// defs shared with qcc
#include "progdefs.h"			// generated by program cdefs

typedef union eval_s
{
	string_t		string;
	float			_float;
	float			vector[3];
	func_t			function;
	int				ivector[3];
	int				_int;
	int				edict;
} prvm_eval_t;

typedef struct link_s
{
	int entitynumber;
	struct link_s	*prev, *next;
} link_t;

#define ENTITYGRIDAREAS 16

typedef struct edict_engineprivate_s
{
	// true if this edict is unused
	qboolean free;
	// sv.time when the object was freed (to prevent early reuse which could
	// mess up client interpolation or obscure severe QuakeC bugs)
	float freetime;

	// cached cluster links for quick stationary object visibility checking
	vec3_t cullmins, cullmaxs;
	int pvs_numclusters;
	int pvs_clusterlist[MAX_ENTITYCLUSTERS];

	// physics grid areas this edict is linked into
	link_t areagrid[ENTITYGRIDAREAS];
	// since the areagrid can have multiple references to one entity,
	// we should avoid extensive checking on entities already encountered
	int areagridmarknumber;

	// PROTOCOL_QUAKE, PROTOCOL_QUAKEDP, PROTOCOL_NEHAHRAMOVIE, PROTOCOL_QUAKEWORLD
	// baseline values
	entity_state_t baseline;

	// LordHavoc: gross hack to make floating items still work
	int suspendedinairflag;
	// used by PushMove to keep track of where objects were before they were
	// moved, in case they need to be moved back
	vec3_t moved_from;
	vec3_t moved_fromangles;
}
edict_engineprivate_t;

// the entire server entity structure
// NOTE: keep this small!  priv and v are dynamic but this struct is not!
typedef struct edict_s
{
	// engine-private fields (stored in dynamically resized array)
	edict_engineprivate_t *e;
	// QuakeC fields (stored in dynamically resized array)
	entvars_t *v;
}
prvm_edict_t;

// LordHavoc: in an effort to eliminate time wasted on GetEdictFieldValue...  see pr_edict.c for the functions which use these.
extern int eval_gravity;
extern int eval_button3;
extern int eval_button4;
extern int eval_button5;
extern int eval_button6;
extern int eval_button7;
extern int eval_button8;
extern int eval_buttonuse;
extern int eval_buttonchat;
extern int eval_glow_size;
extern int eval_glow_trail;
extern int eval_glow_color;
extern int eval_items2;
extern int eval_scale;
extern int eval_alpha;
extern int eval_renderamt; // HalfLife support
extern int eval_rendermode; // HalfLife support
extern int eval_fullbright;
extern int eval_ammo_shells1;
extern int eval_ammo_nails1;
extern int eval_ammo_lava_nails;
extern int eval_ammo_rockets1;
extern int eval_ammo_multi_rockets;
extern int eval_ammo_cells1;
extern int eval_ammo_plasma;
extern int eval_idealpitch;
extern int eval_pitch_speed;
extern int eval_viewmodelforclient;
extern int eval_nodrawtoclient;
extern int eval_exteriormodeltoclient;
extern int eval_drawonlytoclient;
extern int eval_ping;
extern int eval_movement;
extern int eval_pmodel;
extern int eval_punchvector;
extern int eval_viewzoom;
extern int eval_clientcolors;
extern int eval_tag_entity;
extern int eval_tag_index;
extern int eval_light_lev;
extern int eval_color;
extern int eval_style;
extern int eval_pflags;
extern int eval_cursor_active;
extern int eval_cursor_screen;
extern int eval_cursor_trace_start;
extern int eval_cursor_trace_endpos;
extern int eval_cursor_trace_ent;
extern int eval_colormod;
extern int eval_playermodel;
extern int eval_playerskin;

#define PRVM_GETEDICTFIELDVALUE(ed, fieldoffset) (fieldoffset ? (prvm_eval_t *)((unsigned char *)ed->v + fieldoffset) : NULL)

extern mfunction_t *SV_PlayerPhysicsQC;
extern mfunction_t *EndFrameQC;
//KrimZon - SERVER COMMANDS IN QUAKEC
extern mfunction_t *SV_ParseClientCommandQC;

//============================================================================

extern	dprograms_t		*progs;
extern	mfunction_t		*prog->functions;
extern	char			*pr_strings;
extern	int				pr_stringssize;
extern	ddef_t			*pr_globaldefs;
extern	ddef_t			*pr_fielddefs;
extern	dstatement_t	*pr_statements;
extern	globalvars_t	*pr_global_struct;
extern	float			*pr_globals;			// same as pr_global_struct

extern	int				prog->edict_size;	// in bytes
extern	int				pr_edictareasize; // LordHavoc: for bounds checking

extern	int				pr_maxknownstrings;
extern	int				pr_numknownstrings;
extern	const char		**pr_knownstrings;

//============================================================================

void PR_Init (void);
void PR_Shutdown (void);

void PRVM_ExecuteProgram (func_t fnum, const char *errormessage);
void PR_LoadProgs (const char *progsname);

#define PR_Alloc(buffersize) _PR_Alloc(buffersize, __FILE__, __LINE__)
#define PR_Free(buffer) _PR_Free(buffer, __FILE__, __LINE__)
#define PR_FreeAll() _PR_FreeAll(__FILE__, __LINE__)
void *_PR_Alloc (size_t buffersize, const char *filename, int fileline);
void _PR_Free (void *buffer, const char *filename, int fileline);
void _PR_FreeAll (const char *filename, int fileline);

void PR_Profile_f (void);

void PR_PrintState(void);
void PR_Crash (void);

void SV_IncreaseEdicts(void);

prvm_edict_t *ED_Alloc (void);
void ED_Free (prvm_edict_t *ed);
void ED_ClearEdict (prvm_edict_t *e);

void ED_Print(prvm_edict_t *ed);
void ED_Write (qfile_t *f, prvm_edict_t *ed);
const char *ED_ParseEdict (const char *data, prvm_edict_t *ent);

void ED_WriteGlobals (qfile_t *f);
void ED_ParseGlobals (const char *data);

void ED_LoadFromFile (const char *data);

prvm_edict_t *EDICT_NUM_ERROR(int n, char *filename, int fileline);
#define PRVM_EDICT_NUM(n) (((n) >= 0 && (n) < prog->max_edicts) ? prog->edicts + (n) : EDICT_NUM_ERROR(n, __FILE__, __LINE__))
#define EDICT_NUM_UNSIGNED(n) (((n) < prog->max_edicts) ? prog->edicts + (n) : EDICT_NUM_ERROR(n, __FILE__, __LINE__))

//int NUM_FOR_EDICT_ERROR(prvm_edict_t *e);
#define PRVM_NUM_FOR_EDICT(e) ((int)((prvm_edict_t *)(e) - prog->edicts))
//int PRVM_NUM_FOR_EDICT(prvm_edict_t *e);

#define	PRVM_NEXT_EDICT(e) ((e) + 1)

#define PRVM_EDICT_TO_PROG(e) (PRVM_NUM_FOR_EDICT(e))
//int PRVM_EDICT_TO_PROG(prvm_edict_t *e);
#define PRVM_PROG_TO_EDICT(n) (PRVM_EDICT_NUM(n))
//prvm_edict_t *PRVM_PROG_TO_EDICT(int n);

//============================================================================

#define	PRVM_G_FLOAT(o) (pr_globals[o])
#define	PRVM_G_INT(o) (*(int *)&pr_globals[o])
#define	PRVM_G_EDICT(o) (PRVM_PROG_TO_EDICT(*(int *)&pr_globals[o]))
#define PRVM_G_EDICTNUM(o) PRVM_NUM_FOR_EDICT(PRVM_G_EDICT(o))
#define	PRVM_G_VECTOR(o) (&pr_globals[o])
#define	PRVM_G_STRING(o) (PRVM_GetString(*(string_t *)&pr_globals[o]))
//#define	G_FUNCTION(o) (*(func_t *)&pr_globals[o])

// FIXME: make these go away?
#define	E_FLOAT(e,o) (((float*)e->v)[o])
//#define	E_INT(e,o) (((int*)e->v)[o])
//#define	E_VECTOR(e,o) (&((float*)e->v)[o])
#define	E_STRING(e,o) (PRVM_GetString(*(string_t *)&((float*)e->v)[o]))

extern	int		type_size[8];

typedef void (*builtin_t) (void);
extern	builtin_t *pr_builtins;
extern int pr_numbuiltins;

extern int		pr_argc;

extern	int			pr_trace;
extern	mfunction_t	*pr_xfunction;
extern	int			pr_xstatement;

extern	unsigned short		pr_crc;

void PR_Execute_ProgsLoaded(void);

void ED_PrintEdicts (void);
void ED_PrintNum (int ent);

const char *PRVM_GetString(int num);
int PR_SetQCString(const char *s);
int PRVM_SetEngineString(const char *s);
int PRVM_SetTempString(const char *s);
char *PR_AllocString(int bufferlength);
void PR_FreeString(char *s);

#endif

