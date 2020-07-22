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

#include "quakedef.h"

// for secure rcon authentication
#include "hmac.h"
#include "mdfour.h"
#include "image.h"
#include <time.h>

cvar_t cl_name = {CVAR_CLIENT | CVAR_SAVE | CVAR_USERINFO, "name", "player", "change your player name"};
cvar_t cl_rate = {CVAR_CLIENT | CVAR_SAVE | CVAR_USERINFO, "rate", "20000", "change your connection speed"};
cvar_t cl_rate_burstsize = {CVAR_CLIENT | CVAR_SAVE | CVAR_USERINFO, "rate_burstsize", "1024", "internal storage cvar for current rate control burst size (changed by rate_burstsize command)"};
cvar_t cl_topcolor = {CVAR_CLIENT | CVAR_SAVE | CVAR_USERINFO, "topcolor", "0", "change the color of your shirt"};
cvar_t cl_bottomcolor = {CVAR_CLIENT | CVAR_SAVE | CVAR_USERINFO, "bottomcolor", "0", "change the color of your pants"};
cvar_t cl_team = {CVAR_CLIENT | CVAR_USERINFO | CVAR_SAVE, "team", "none", "QW team (4 character limit, example: blue)"};
cvar_t cl_skin = {CVAR_CLIENT | CVAR_USERINFO | CVAR_SAVE, "skin", "", "QW player skin name (example: base)"};
cvar_t cl_noaim = {CVAR_CLIENT | CVAR_USERINFO | CVAR_SAVE, "noaim", "1", "QW option to disable vertical autoaim"};
cvar_t cl_pmodel = {CVAR_CLIENT | CVAR_USERINFO | CVAR_SAVE, "pmodel", "0", "current player model number in nehahra"};
cvar_t r_fixtrans_auto = {CVAR_CLIENT, "r_fixtrans_auto", "0", "automatically fixtrans textures (when set to 2, it also saves the fixed versions to a fixtrans directory)"};

extern cvar_t rcon_secure;
extern cvar_t rcon_secure_challengetimeout;

/*
===================
CL_ForwardToServer

Sends an entire command string over to the server, unprocessed
===================
*/
void CL_ForwardToServer (const char *s)
{
	char temp[128];
	if (cls.state != ca_connected)
	{
		Con_Printf("Can't \"%s\", not connected\n", s);
		return;
	}

	if (!cls.netcon)
		return;

	// LadyHavoc: thanks to Fuh for bringing the pure evil of SZ_Print to my
	// attention, it has been eradicated from here, its only (former) use in
	// all of darkplaces.
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
		MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
	else
		MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
	if ((!strncmp(s, "say ", 4) || !strncmp(s, "say_team ", 9)) && cl_locs_enable.integer)
	{
		// say/say_team commands can replace % character codes with status info
		while (*s)
		{
			if (*s == '%' && s[1])
			{
				// handle proquake message macros
				temp[0] = 0;
				switch (s[1])
				{
				case 'l': // current location
					CL_Locs_FindLocationName(temp, sizeof(temp), cl.movement_origin);
					break;
				case 'h': // current health
					dpsnprintf(temp, sizeof(temp), "%i", cl.stats[STAT_HEALTH]);
					break;
				case 'a': // current armor
					dpsnprintf(temp, sizeof(temp), "%i", cl.stats[STAT_ARMOR]);
					break;
				case 'x': // current rockets
					dpsnprintf(temp, sizeof(temp), "%i", cl.stats[STAT_ROCKETS]);
					break;
				case 'c': // current cells
					dpsnprintf(temp, sizeof(temp), "%i", cl.stats[STAT_CELLS]);
					break;
				// silly proquake macros
				case 'd': // loc at last death
					CL_Locs_FindLocationName(temp, sizeof(temp), cl.lastdeathorigin);
					break;
				case 't': // current time
					dpsnprintf(temp, sizeof(temp), "%.0f:%.0f", floor(cl.time / 60), cl.time - floor(cl.time / 60) * 60);
					break;
				case 'r': // rocket launcher status ("I have RL", "I need rockets", "I need RL")
					if (!(cl.stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER))
						dpsnprintf(temp, sizeof(temp), "I need RL");
					else if (!cl.stats[STAT_ROCKETS])
						dpsnprintf(temp, sizeof(temp), "I need rockets");
					else
						dpsnprintf(temp, sizeof(temp), "I have RL");
					break;
				case 'p': // powerup status (outputs "quad" "pent" and "eyes" according to status)
					if (cl.stats[STAT_ITEMS] & IT_QUAD)
					{
						if (temp[0])
							strlcat(temp, " ", sizeof(temp));
						strlcat(temp, "quad", sizeof(temp));
					}
					if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
					{
						if (temp[0])
							strlcat(temp, " ", sizeof(temp));
						strlcat(temp, "pent", sizeof(temp));
					}
					if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
					{
						if (temp[0])
							strlcat(temp, " ", sizeof(temp));
						strlcat(temp, "eyes", sizeof(temp));
					}
					break;
				case 'w': // weapon status (outputs "SSG:NG:SNG:GL:RL:LG" with the text between : characters omitted if you lack the weapon)
					if (cl.stats[STAT_ITEMS] & IT_SUPER_SHOTGUN)
						strlcat(temp, "SSG", sizeof(temp));
					strlcat(temp, ":", sizeof(temp));
					if (cl.stats[STAT_ITEMS] & IT_NAILGUN)
						strlcat(temp, "NG", sizeof(temp));
					strlcat(temp, ":", sizeof(temp));
					if (cl.stats[STAT_ITEMS] & IT_SUPER_NAILGUN)
						strlcat(temp, "SNG", sizeof(temp));
					strlcat(temp, ":", sizeof(temp));
					if (cl.stats[STAT_ITEMS] & IT_GRENADE_LAUNCHER)
						strlcat(temp, "GL", sizeof(temp));
					strlcat(temp, ":", sizeof(temp));
					if (cl.stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER)
						strlcat(temp, "RL", sizeof(temp));
					strlcat(temp, ":", sizeof(temp));
					if (cl.stats[STAT_ITEMS] & IT_LIGHTNING)
						strlcat(temp, "LG", sizeof(temp));
					break;
				default:
					// not a recognized macro, print it as-is...
					temp[0] = s[0];
					temp[1] = s[1];
					temp[2] = 0;
					break;
				}
				// write the resulting text
				SZ_Write(&cls.netcon->message, (unsigned char *)temp, (int)strlen(temp));
				s += 2;
				continue;
			}
			MSG_WriteByte(&cls.netcon->message, *s);
			s++;
		}
		MSG_WriteByte(&cls.netcon->message, 0);
	}
	else // any other command is passed on as-is
		SZ_Write(&cls.netcon->message, (const unsigned char *)s, (int)strlen(s) + 1);
}

void CL_ForwardToServer_f (cmd_state_t *cmd)
{
	const char *s;
	char vabuf[MAX_INPUTLINE];
	size_t i;
	if (!strcasecmp(Cmd_Argv(cmd, 0), "cmd"))
	{
		// we want to strip off "cmd", so just send the args
		s = Cmd_Argc(cmd) > 1 ? Cmd_Args(cmd) : "";
	}
	else
	{
		// we need to keep the command name, so send Cmd_Argv(cmd, 0), a space and then Cmd_Args(cmd)
		i = dpsnprintf(vabuf, sizeof(vabuf), "%s", Cmd_Argv(cmd, 0));
		if(Cmd_Argc(cmd) > 1)
			// (i + 1) accounts for the added space
			dpsnprintf(&vabuf[i], sizeof(vabuf) - (i + 1), " %s", Cmd_Args(cmd));
		s = vabuf;
	}
	// don't send an empty forward message if the user tries "cmd" by itself
	if (!s || !*s)
		return;
	CL_ForwardToServer(s);
}

static void CL_SendCvar_f(cmd_state_t *cmd)
{
	cvar_t	*c;
	const char *cvarname;
	char vabuf[1024];

	if(Cmd_Argc(cmd) != 2)
		return;
	cvarname = Cmd_Argv(cmd, 1);
	if (cls.state == ca_connected)
	{
		c = Cvar_FindVar(&cvars_all, cvarname, CVAR_CLIENT | CVAR_SERVER);
		// LadyHavoc: if there is no such cvar or if it is private, send a
		// reply indicating that it has no value
		if(!c || (c->flags & CVAR_PRIVATE))
			CL_ForwardToServer(va(vabuf, sizeof(vabuf), "sentcvar %s", cvarname));
		else
			CL_ForwardToServer(va(vabuf, sizeof(vabuf), "sentcvar %s \"%s\"", c->name, c->string));
		return;
	}
}

/*
==================
CL_Color_f
==================
*/
cvar_t cl_color = {CVAR_CLIENT | CVAR_SAVE, "_cl_color", "0", "internal storage cvar for current player colors (changed by color command)"};

// Ignore the callbacks so this two-to-three way synchronization doesn't cause an infinite loop.
static void CL_Color_c(cvar_t *var)
{
	char vabuf[1024];
	
	Cvar_Set_NoCallback(&cl_topcolor, va(vabuf, sizeof(vabuf), "%i", ((var->integer >> 4) & 15)));
	Cvar_Set_NoCallback(&cl_bottomcolor, va(vabuf, sizeof(vabuf), "%i", (var->integer & 15)));
}

static void CL_Topcolor_c(cvar_t *var)
{
	char vabuf[1024];
	
	Cvar_Set_NoCallback(&cl_color, va(vabuf, sizeof(vabuf), "%i", var->integer*16 + cl_bottomcolor.integer));
}

static void CL_Bottomcolor_c(cvar_t *var)
{
	char vabuf[1024];

	Cvar_Set_NoCallback(&cl_color, va(vabuf, sizeof(vabuf), "%i", cl_topcolor.integer*16 + var->integer));
}

static void CL_Color_f(cmd_state_t *cmd)
{
	int top, bottom;

	if (Cmd_Argc(cmd) == 1)
	{
		if (cmd->source == src_command)
		{
			Con_Printf("\"color\" is \"%i %i\"\n", cl_topcolor.integer, cl_bottomcolor.integer);
			Con_Print("color <0-15> [0-15]\n");
		}
		return;
	}

	if (Cmd_Argc(cmd) == 2)
		top = bottom = atoi(Cmd_Argv(cmd, 1));
	else
	{
		top = atoi(Cmd_Argv(cmd, 1));
		bottom = atoi(Cmd_Argv(cmd, 2));
	}
	/*
	 * This is just a convenient way to change topcolor and bottomcolor
	 * We can't change cl_color from here directly because topcolor and
	 * bottomcolor may be changed separately and do not call this function.
	 * So it has to be changed when the userinfo strings are updated, which
	 * happens twice here. Perhaps find a cleaner way?
	 */

	top = top >= 0 ? top : cl_topcolor.integer;
	bottom = bottom >= 0 ? bottom : cl_bottomcolor.integer;

	top &= 15;
	bottom &= 15;

	// LadyHavoc: allowing skin colormaps 14 and 15 by commenting this out
	//if (top > 13)
	//	top = 13;
	//if (bottom > 13)
	//	bottom = 13;

	if (cmd->source == src_command)
	{
		Cvar_SetValueQuick(&cl_topcolor, top);
		Cvar_SetValueQuick(&cl_bottomcolor, bottom);
		return;
	}
}

/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
static void CL_Packet_f(cmd_state_t *cmd) // credit: taken from QuakeWorld
{
	char send[2048];
	int i, l;
	const char *in;
	char *out;
	lhnetaddress_t address;
	lhnetsocket_t *mysocket;

	if (Cmd_Argc(cmd) != 3)
	{
		Con_Printf ("packet <destination> <contents>\n");
		return;
	}

	if (!LHNETADDRESS_FromString (&address, Cmd_Argv(cmd, 1), sv_netport.integer))
	{
		Con_Printf ("Bad address\n");
		return;
	}

	in = Cmd_Argv(cmd, 2);
	out = send+4;
	send[0] = send[1] = send[2] = send[3] = -1;

	l = (int)strlen (in);
	for (i=0 ; i<l ; i++)
	{
		if (out >= send + sizeof(send) - 1)
			break;
		if (in[i] == '\\' && in[i+1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == '0')
		{
			*out++ = '\0';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == 't')
		{
			*out++ = '\t';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == 'r')
		{
			*out++ = '\r';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == '"')
		{
			*out++ = '\"';
			i++;
		}
		else
			*out++ = in[i];
	}

	mysocket = NetConn_ChooseClientSocketForAddress(&address);
	if (!mysocket)
		mysocket = NetConn_ChooseServerSocketForAddress(&address);
	if (mysocket)
		NetConn_Write(mysocket, send, out - send, &address);
}

/*
=====================
CL_PQRcon_f

ProQuake rcon support
=====================
*/
static void CL_PQRcon_f(cmd_state_t *cmd)
{
	int n;
	const char *e;
	lhnetsocket_t *mysocket;

	if (Cmd_Argc(cmd) == 1)
	{
		Con_Printf("%s: Usage: %s command\n", Cmd_Argv(cmd, 0), Cmd_Argv(cmd, 0));
		return;
	}

	if (!rcon_password.string || !rcon_password.string[0] || rcon_secure.integer > 0)
	{
		Con_Printf ("You must set rcon_password before issuing an pqrcon command, and rcon_secure must be 0.\n");
		return;
	}

	e = strchr(rcon_password.string, ' ');
	n = e ? e-rcon_password.string : (int)strlen(rcon_password.string);

	if (cls.netcon)
		cls.rcon_address = cls.netcon->peeraddress;
	else
	{
		if (!rcon_address.string[0])
		{
			Con_Printf ("You must either be connected, or set the rcon_address cvar to issue rcon commands\n");
			return;
		}
		LHNETADDRESS_FromString(&cls.rcon_address, rcon_address.string, sv_netport.integer);
	}
	mysocket = NetConn_ChooseClientSocketForAddress(&cls.rcon_address);
	if (mysocket)
	{
		sizebuf_t buf;
		unsigned char bufdata[64];
		buf.data = bufdata;
		SZ_Clear(&buf);
		MSG_WriteLong(&buf, 0);
		MSG_WriteByte(&buf, CCREQ_RCON);
		SZ_Write(&buf, (const unsigned char*)rcon_password.string, n);
		MSG_WriteByte(&buf, 0); // terminate the (possibly partial) string
		MSG_WriteString(&buf, Cmd_Args(cmd));
		StoreBigLong(buf.data, NETFLAG_CTL | (buf.cursize & NETFLAG_LENGTH_MASK));
		NetConn_Write(mysocket, buf.data, buf.cursize, &cls.rcon_address);
		SZ_Clear(&buf);
	}
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
static void CL_Rcon_f(cmd_state_t *cmd) // credit: taken from QuakeWorld
{
	int i, n;
	const char *e;
	lhnetsocket_t *mysocket;

	if (Cmd_Argc(cmd) == 1)
	{
		Con_Printf("%s: Usage: %s command\n", Cmd_Argv(cmd, 0), Cmd_Argv(cmd, 0));
		return;
	}

	if (!rcon_password.string || !rcon_password.string[0])
	{
		Con_Printf ("You must set rcon_password before issuing an rcon command.\n");
		return;
	}

	e = strchr(rcon_password.string, ' ');
	n = e ? e-rcon_password.string : (int)strlen(rcon_password.string);

	if (cls.netcon)
		cls.rcon_address = cls.netcon->peeraddress;
	else
	{
		if (!rcon_address.string[0])
		{
			Con_Printf ("You must either be connected, or set the rcon_address cvar to issue rcon commands\n");
			return;
		}
		LHNETADDRESS_FromString(&cls.rcon_address, rcon_address.string, sv_netport.integer);
	}
	mysocket = NetConn_ChooseClientSocketForAddress(&cls.rcon_address);
	if (mysocket && Cmd_Args(cmd)[0])
	{
		// simply put together the rcon packet and send it
		if(Cmd_Argv(cmd, 0)[0] == 's' || rcon_secure.integer > 1)
		{
			if(cls.rcon_commands[cls.rcon_ringpos][0])
			{
				char s[128];
				LHNETADDRESS_ToString(&cls.rcon_addresses[cls.rcon_ringpos], s, sizeof(s), true);
				Con_Printf("rcon to %s (for command %s) failed: too many buffered commands (possibly increase MAX_RCONS)\n", s, cls.rcon_commands[cls.rcon_ringpos]);
				cls.rcon_commands[cls.rcon_ringpos][0] = 0;
				--cls.rcon_trying;
			}
			for (i = 0;i < MAX_RCONS;i++)
				if(cls.rcon_commands[i][0])
					if (!LHNETADDRESS_Compare(&cls.rcon_address, &cls.rcon_addresses[i]))
						break;
			++cls.rcon_trying;
			if(i >= MAX_RCONS)
				NetConn_WriteString(mysocket, "\377\377\377\377getchallenge", &cls.rcon_address); // otherwise we'll request the challenge later
			strlcpy(cls.rcon_commands[cls.rcon_ringpos], Cmd_Args(cmd), sizeof(cls.rcon_commands[cls.rcon_ringpos]));
			cls.rcon_addresses[cls.rcon_ringpos] = cls.rcon_address;
			cls.rcon_timeout[cls.rcon_ringpos] = host.realtime + rcon_secure_challengetimeout.value;
			cls.rcon_ringpos = (cls.rcon_ringpos + 1) % MAX_RCONS;
		}
		else if(rcon_secure.integer > 0)
		{
			char buf[1500];
			char argbuf[1500];
			dpsnprintf(argbuf, sizeof(argbuf), "%ld.%06d %s", (long) time(NULL), (int) (rand() % 1000000), Cmd_Args(cmd));
			memcpy(buf, "\377\377\377\377srcon HMAC-MD4 TIME ", 24);
			if(HMAC_MDFOUR_16BYTES((unsigned char *) (buf + 24), (unsigned char *) argbuf, (int)strlen(argbuf), (unsigned char *) rcon_password.string, n))
			{
				buf[40] = ' ';
				strlcpy(buf + 41, argbuf, sizeof(buf) - 41);
				NetConn_Write(mysocket, buf, 41 + (int)strlen(buf + 41), &cls.rcon_address);
			}
		}
		else
		{
			char buf[1500];
			memcpy(buf, "\377\377\377\377", 4);
			dpsnprintf(buf+4, sizeof(buf)-4, "rcon %.*s %s",  n, rcon_password.string, Cmd_Args(cmd));
			NetConn_WriteString(mysocket, buf, &cls.rcon_address);
		}
	}
}

/*
==================
CL_FullServerinfo_f

Sent by server when serverinfo changes
==================
*/
// TODO: shouldn't this be a cvar instead?
static void CL_FullServerinfo_f(cmd_state_t *cmd) // credit: taken from QuakeWorld
{
	char temp[512];
	if (Cmd_Argc(cmd) != 2)
	{
		Con_Printf ("usage: fullserverinfo <complete info string>\n");
		return;
	}

	strlcpy (cl.qw_serverinfo, Cmd_Argv(cmd, 1), sizeof(cl.qw_serverinfo));
	InfoString_GetValue(cl.qw_serverinfo, "teamplay", temp, sizeof(temp));
	cl.qw_teamplay = atoi(temp);
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
Casey was here :)
*/
static void CL_FullInfo_f(cmd_state_t *cmd) // credit: taken from QuakeWorld
{
	char key[512];
	char value[512];
	const char *s;

	if (Cmd_Argc(cmd) != 2)
	{
		Con_Printf ("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(cmd, 1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		size_t len = strcspn(s, "\\");
		if (len >= sizeof(key)) {
			len = sizeof(key) - 1;
		}
		strlcpy(key, s, len + 1);
		s += len;
		if (!*s)
		{
			Con_Printf ("MISSING VALUE\n");
			return;
		}
		++s; // Skip over backslash.

		len = strcspn(s, "\\");
		if (len >= sizeof(value)) {
			len = sizeof(value) - 1;
		}
		strlcpy(value, s, len + 1);

		CL_SetInfo(key, value, false, false, false, false);

		s += len;
		if (!*s)
		{
			break;
		}
		++s; // Skip over backslash.
	}
}

/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
static void CL_SetInfo_f(cmd_state_t *cmd) // credit: taken from QuakeWorld
{
	if (Cmd_Argc(cmd) == 1)
	{
		InfoString_Print(cls.userinfo);
		return;
	}
	if (Cmd_Argc(cmd) != 3)
	{
		Con_Printf ("usage: setinfo [ <key> <value> ]\n");
		return;
	}
	CL_SetInfo(Cmd_Argv(cmd, 1), Cmd_Argv(cmd, 2), true, false, false, false);
}

static void CL_PingPLReport_f(cmd_state_t *cmd)
{
	char *errbyte;
	int i;
	int l = Cmd_Argc(cmd);
	if (l > cl.maxclients)
		l = cl.maxclients;
	for (i = 0;i < l;i++)
	{
		cl.scores[i].qw_ping = atoi(Cmd_Argv(cmd, 1+i*2));
		cl.scores[i].qw_packetloss = strtol(Cmd_Argv(cmd, 1+i*2+1), &errbyte, 0);
		if(errbyte && *errbyte == ',')
			cl.scores[i].qw_movementloss = atoi(errbyte + 1);
		else
			cl.scores[i].qw_movementloss = 0;
	}
}

void CL_InitCommands(void)
{
	dpsnprintf(cls.userinfo, sizeof(cls.userinfo), "\\name\\player\\team\\none\\topcolor\\0\\bottomcolor\\0\\rate\\10000\\msg\\1\\noaim\\1\\*ver\\dp");

	Cvar_RegisterVariable(&cl_name);
	Cvar_RegisterAlias(&cl_name, "_cl_name");
	Cvar_RegisterVariable(&cl_rate);
	Cvar_RegisterAlias(&cl_rate, "_cl_rate");
	Cvar_RegisterVariable(&cl_rate_burstsize);
	Cvar_RegisterAlias(&cl_rate_burstsize, "_cl_rate_burstsize");
	Cvar_RegisterVariable(&cl_pmodel);
	Cvar_RegisterAlias(&cl_pmodel, "_cl_pmodel");
	Cvar_RegisterVariable(&cl_color);
	Cvar_RegisterCallback(&cl_color, CL_Color_c);
	Cvar_RegisterVariable(&cl_topcolor);
	Cvar_RegisterCallback(&cl_topcolor, CL_Topcolor_c);
	Cvar_RegisterVariable(&cl_bottomcolor);
	Cvar_RegisterCallback(&cl_bottomcolor, CL_Bottomcolor_c);
	Cvar_RegisterVariable(&r_fixtrans_auto);
	Cvar_RegisterVariable(&cl_team);
	Cvar_RegisterVariable(&cl_skin);
	Cvar_RegisterVariable(&cl_noaim);	

	Cmd_AddCommand(CMD_CLIENT | CMD_CLIENT_FROM_SERVER, "cmd", CL_ForwardToServer_f, "send a console commandline to the server (used by some mods)");
	Cmd_AddCommand(CMD_CLIENT, "color", CL_Color_f, "change your player shirt and pants colors");
	Cmd_AddCommand(CMD_CLIENT, "rcon", CL_Rcon_f, "sends a command to the server console (if your rcon_password matches the server's rcon_password), or to the address specified by rcon_address when not connected (again rcon_password must match the server's); note: if rcon_secure is set, client and server clocks must be synced e.g. via NTP");
	Cmd_AddCommand(CMD_CLIENT, "srcon", CL_Rcon_f, "sends a command to the server console (if your rcon_password matches the server's rcon_password), or to the address specified by rcon_address when not connected (again rcon_password must match the server's); this always works as if rcon_secure is set; note: client and server clocks must be synced e.g. via NTP");
	Cmd_AddCommand(CMD_CLIENT, "pqrcon", CL_PQRcon_f, "sends a command to a proquake server console (if your rcon_password matches the server's rcon_password), or to the address specified by rcon_address when not connected (again rcon_password must match the server's)");
	Cmd_AddCommand(CMD_CLIENT, "packet", CL_Packet_f, "send a packet to the specified address:port containing a text string");
	Cmd_AddCommand(CMD_CLIENT, "fullinfo", CL_FullInfo_f, "allows client to modify their userinfo");
	Cmd_AddCommand(CMD_CLIENT, "setinfo", CL_SetInfo_f, "modifies your userinfo");
	Cmd_AddCommand(CMD_CLIENT, "sendcvar", CL_SendCvar_f, "sends the value of a cvar to the server as a sentcvar command, for use by QuakeC");
	Cmd_AddCommand(CMD_CLIENT, "fixtrans", Image_FixTransparentPixels_f, "change alpha-zero pixels in an image file to sensible values, and write out a new TGA (warning: SLOW)");

	// commands that are only sent by server to client for execution
	Cmd_AddCommand(CMD_CLIENT_FROM_SERVER, "pingplreport", CL_PingPLReport_f, "command sent by server containing client ping and packet loss values for scoreboard, triggered by pings command from client (not used by QW servers)");
	Cmd_AddCommand(CMD_CLIENT_FROM_SERVER, "fullserverinfo", CL_FullServerinfo_f, "internal use only, sent by server to client to update client's local copy of serverinfo string");
}
