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
// wad.c

#include "quakedef.h"

int			wad_numlumps;
lumpinfo_t	*wad_lumps;
byte		*wad_base;

void SwapPic (qpic_t *pic);

/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName (char *in, char *out)
{
	int		i;
	int		c;
	
	for (i=0 ; i<16 ; i++ )
	{
		c = in[i];
		if (!c)
			break;
			
		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		out[i] = c;
	}
	
	for ( ; i< 16 ; i++ )
		out[i] = 0;
}



/*
====================
W_LoadWadFile
====================
*/
void W_LoadWadFile (char *filename)
{
	lumpinfo_t		*lump_p;
	wadinfo_t		*header;
	unsigned		i;
	int				infotableofs;
	
	wad_base = COM_LoadHunkFile (filename, false);
	if (!wad_base)
		Sys_Error ("W_LoadWadFile: couldn't load %s", filename);

	header = (wadinfo_t *)wad_base;
	
	if (header->identification[0] != 'W'
	|| header->identification[1] != 'A'
	|| header->identification[2] != 'D'
	|| header->identification[3] != '2')
		Sys_Error ("Wad file %s doesn't have WAD2 id\n",filename);
		
	wad_numlumps = LittleLong(header->numlumps);
	infotableofs = LittleLong(header->infotableofs);
	wad_lumps = (lumpinfo_t *)(wad_base + infotableofs);
	
	for (i=0, lump_p = wad_lumps ; i<wad_numlumps ; i++,lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName (lump_p->name, lump_p->name);
		if (lump_p->type == TYP_QPIC)
			SwapPic ( (qpic_t *)(wad_base + lump_p->filepos));
	}
}


/*
=============
W_GetLumpinfo
=============
*/
lumpinfo_t	*W_GetLumpinfo (char *name)
{
	int		i;
	lumpinfo_t	*lump_p;
	char	clean[16];
	
	W_CleanupName (name, clean);
	
	for (lump_p=wad_lumps, i=0 ; i<wad_numlumps ; i++,lump_p++)
	{
		if (!strcmp(clean, lump_p->name))
			return lump_p;
	}
	
	Sys_Error ("W_GetLumpinfo: %s not found", name);
	return NULL;
}

void *W_GetLumpName (char *name)
{
	lumpinfo_t	*lump;
	
	lump = W_GetLumpinfo (name);
	
	return (void *)(wad_base + lump->filepos);
}

void *W_GetLumpNum (int num)
{
	lumpinfo_t	*lump;
	
	if (num < 0 || num > wad_numlumps)
		Sys_Error ("W_GetLumpNum: bad number: %i", num);
		
	lump = wad_lumps + num;
	
	return (void *)(wad_base + lump->filepos);
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic (qpic_t *pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);	
}

// LordHavoc: added alternate WAD2/WAD3 system for HalfLife texture wads
#define TEXWAD_MAXIMAGES 16384
typedef struct
{
	char name[16];
	FILE *file;
	int position;
	int size;
} texwadlump_t;

texwadlump_t	texwadlump[TEXWAD_MAXIMAGES];

/*
====================
W_LoadTextureWadFile
====================
*/
void W_LoadTextureWadFile (char *filename, int complain)
{
	lumpinfo_t		*lumps, *lump_p;
	wadinfo_t		header;
	unsigned		i, j;
	int				infotableofs;
	FILE			*file;
	int				numlumps;
	
	COM_FOpenFile (filename, &file, false);
	if (!file)
	{
		if (complain)
			Con_Printf ("W_LoadTextureWadFile: couldn't find %s", filename);
		return;
	}

	if (fread(&header, sizeof(wadinfo_t), 1, file) != 1)
	{Con_Printf ("W_LoadTextureWadFile: unable to read wad header");return;}
	
	if(header.identification[0] != 'W'
	|| header.identification[1] != 'A'
	|| header.identification[2] != 'D'
	|| header.identification[3] != '3')
	{Con_Printf ("W_LoadTextureWadFile: Wad file %s doesn't have WAD3 id\n",filename);return;}

	numlumps = LittleLong(header.numlumps);
	if (numlumps < 1 || numlumps > TEXWAD_MAXIMAGES)
	{Con_Printf ("W_LoadTextureWadFile: invalid number of lumps (%i)\n", numlumps);return;}
	infotableofs = LittleLong(header.infotableofs);
	if (fseek(file, infotableofs, SEEK_SET))
	{Con_Printf ("W_LoadTextureWadFile: unable to seek to lump table");return;}
	if (!(lumps = qmalloc(sizeof(lumpinfo_t)*numlumps)))
	{Con_Printf ("W_LoadTextureWadFile: unable to allocate temporary memory for lump table");return;}

	if (fread(lumps, sizeof(lumpinfo_t), numlumps, file) != numlumps)
	{Con_Printf ("W_LoadTextureWadFile: unable to read lump table");return;}
	
	for (i=0, lump_p = lumps ; i<numlumps ; i++,lump_p++)
	{
		W_CleanupName (lump_p->name, lump_p->name);
		for (j = 0;j < TEXWAD_MAXIMAGES;j++)
		{
			if (texwadlump[j].name[0]) // occupied slot, check the name
			{
				if (!strcmp(lump_p->name, texwadlump[j].name)) // name match, replace old one
					break;
			}
			else // empty slot
				break;
		}
		if (j >= TEXWAD_MAXIMAGES)
			break; // abort loading
		W_CleanupName (lump_p->name, texwadlump[j].name);
		texwadlump[j].file = file;
		texwadlump[j].position = LittleLong(lump_p->filepos);
		texwadlump[j].size = LittleLong(lump_p->disksize);
	}
	qfree(lumps);
	// leaves the file open
}


byte hlpalette[768] =
{
	0x00,0x00,0x00,0x0F,0x0F,0x0F,0x1F,0x1F,0x1F,0x2F,0x2F,0x2F,0x3F,0x3F,0x3F,0x4B,
	0x4B,0x4B,0x5B,0x5B,0x5B,0x6B,0x6B,0x6B,0x7B,0x7B,0x7B,0x8B,0x8B,0x8B,0x9B,0x9B,
	0x9B,0xAB,0xAB,0xAB,0xBB,0xBB,0xBB,0xCB,0xCB,0xCB,0xDB,0xDB,0xDB,0xEB,0xEB,0xEB,
	0x0F,0x0B,0x07,0x17,0x0F,0x0B,0x1F,0x17,0x0B,0x27,0x1B,0x0F,0x2F,0x23,0x13,0x37,
	0x2B,0x17,0x3F,0x2F,0x17,0x4B,0x37,0x1B,0x53,0x3B,0x1B,0x5B,0x43,0x1F,0x63,0x4B,
	0x1F,0x6B,0x53,0x1F,0x73,0x57,0x1F,0x7B,0x5F,0x23,0x83,0x67,0x23,0x8F,0x6F,0x23,
	0x0B,0x0B,0x0F,0x13,0x13,0x1B,0x1B,0x1B,0x27,0x27,0x27,0x33,0x2F,0x2F,0x3F,0x37,
	0x37,0x4B,0x3F,0x3F,0x57,0x47,0x47,0x67,0x4F,0x4F,0x73,0x5B,0x5B,0x7F,0x63,0x63,
	0x8B,0x6B,0x6B,0x97,0x73,0x73,0xA3,0x7B,0x7B,0xAF,0x83,0x83,0xBB,0x8B,0x8B,0xCB,
	0x00,0x00,0x00,0x07,0x07,0x00,0x0B,0x0B,0x00,0x13,0x13,0x00,0x1B,0x1B,0x00,0x23,
	0x23,0x00,0x2B,0x2B,0x07,0x2F,0x2F,0x07,0x37,0x37,0x07,0x3F,0x3F,0x07,0x47,0x47,
	0x07,0x4B,0x4B,0x0B,0x53,0x53,0x0B,0x5B,0x5B,0x0B,0x63,0x63,0x0B,0x6B,0x6B,0x0F,
	0x07,0x00,0x00,0x0F,0x00,0x00,0x17,0x00,0x00,0x1F,0x00,0x00,0x27,0x00,0x00,0x2F,
	0x00,0x00,0x37,0x00,0x00,0x3F,0x00,0x00,0x47,0x00,0x00,0x4F,0x00,0x00,0x57,0x00,
	0x00,0x5F,0x00,0x00,0x67,0x00,0x00,0x6F,0x00,0x00,0x77,0x00,0x00,0x7F,0x00,0x00,
	0x13,0x13,0x00,0x1B,0x1B,0x00,0x23,0x23,0x00,0x2F,0x2B,0x00,0x37,0x2F,0x00,0x43,
	0x37,0x00,0x4B,0x3B,0x07,0x57,0x43,0x07,0x5F,0x47,0x07,0x6B,0x4B,0x0B,0x77,0x53,
	0x0F,0x83,0x57,0x13,0x8B,0x5B,0x13,0x97,0x5F,0x1B,0xA3,0x63,0x1F,0xAF,0x67,0x23,
	0x23,0x13,0x07,0x2F,0x17,0x0B,0x3B,0x1F,0x0F,0x4B,0x23,0x13,0x57,0x2B,0x17,0x63,
	0x2F,0x1F,0x73,0x37,0x23,0x7F,0x3B,0x2B,0x8F,0x43,0x33,0x9F,0x4F,0x33,0xAF,0x63,
	0x2F,0xBF,0x77,0x2F,0xCF,0x8F,0x2B,0xDF,0xAB,0x27,0xEF,0xCB,0x1F,0xFF,0xF3,0x1B,
	0x0B,0x07,0x00,0x1B,0x13,0x00,0x2B,0x23,0x0F,0x37,0x2B,0x13,0x47,0x33,0x1B,0x53,
	0x37,0x23,0x63,0x3F,0x2B,0x6F,0x47,0x33,0x7F,0x53,0x3F,0x8B,0x5F,0x47,0x9B,0x6B,
	0x53,0xA7,0x7B,0x5F,0xB7,0x87,0x6B,0xC3,0x93,0x7B,0xD3,0xA3,0x8B,0xE3,0xB3,0x97,
	0xAB,0x8B,0xA3,0x9F,0x7F,0x97,0x93,0x73,0x87,0x8B,0x67,0x7B,0x7F,0x5B,0x6F,0x77,
	0x53,0x63,0x6B,0x4B,0x57,0x5F,0x3F,0x4B,0x57,0x37,0x43,0x4B,0x2F,0x37,0x43,0x27,
	0x2F,0x37,0x1F,0x23,0x2B,0x17,0x1B,0x23,0x13,0x13,0x17,0x0B,0x0B,0x0F,0x07,0x07,
	0xBB,0x73,0x9F,0xAF,0x6B,0x8F,0xA3,0x5F,0x83,0x97,0x57,0x77,0x8B,0x4F,0x6B,0x7F,
	0x4B,0x5F,0x73,0x43,0x53,0x6B,0x3B,0x4B,0x5F,0x33,0x3F,0x53,0x2B,0x37,0x47,0x23,
	0x2B,0x3B,0x1F,0x23,0x2F,0x17,0x1B,0x23,0x13,0x13,0x17,0x0B,0x0B,0x0F,0x07,0x07,
	0xDB,0xC3,0xBB,0xCB,0xB3,0xA7,0xBF,0xA3,0x9B,0xAF,0x97,0x8B,0xA3,0x87,0x7B,0x97,
	0x7B,0x6F,0x87,0x6F,0x5F,0x7B,0x63,0x53,0x6B,0x57,0x47,0x5F,0x4B,0x3B,0x53,0x3F,
	0x33,0x43,0x33,0x27,0x37,0x2B,0x1F,0x27,0x1F,0x17,0x1B,0x13,0x0F,0x0F,0x0B,0x07,
	0x6F,0x83,0x7B,0x67,0x7B,0x6F,0x5F,0x73,0x67,0x57,0x6B,0x5F,0x4F,0x63,0x57,0x47,
	0x5B,0x4F,0x3F,0x53,0x47,0x37,0x4B,0x3F,0x2F,0x43,0x37,0x2B,0x3B,0x2F,0x23,0x33,
	0x27,0x1F,0x2B,0x1F,0x17,0x23,0x17,0x0F,0x1B,0x13,0x0B,0x13,0x0B,0x07,0x0B,0x07,
	0xFF,0xF3,0x1B,0xEF,0xDF,0x17,0xDB,0xCB,0x13,0xCB,0xB7,0x0F,0xBB,0xA7,0x0F,0xAB,
	0x97,0x0B,0x9B,0x83,0x07,0x8B,0x73,0x07,0x7B,0x63,0x07,0x6B,0x53,0x00,0x5B,0x47,
	0x00,0x4B,0x37,0x00,0x3B,0x2B,0x00,0x2B,0x1F,0x00,0x1B,0x0F,0x00,0x0B,0x07,0x00,
	0x00,0x00,0xFF,0x0B,0x0B,0xEF,0x13,0x13,0xDF,0x1B,0x1B,0xCF,0x23,0x23,0xBF,0x2B,
	0x2B,0xAF,0x2F,0x2F,0x9F,0x2F,0x2F,0x8F,0x2F,0x2F,0x7F,0x2F,0x2F,0x6F,0x2F,0x2F,
	0x5F,0x2B,0x2B,0x4F,0x23,0x23,0x3F,0x1B,0x1B,0x2F,0x13,0x13,0x1F,0x0B,0x0B,0x0F,
	0x2B,0x00,0x00,0x3B,0x00,0x00,0x4B,0x07,0x00,0x5F,0x07,0x00,0x6F,0x0F,0x00,0x7F,
	0x17,0x07,0x93,0x1F,0x07,0xA3,0x27,0x0B,0xB7,0x33,0x0F,0xC3,0x4B,0x1B,0xCF,0x63,
	0x2B,0xDB,0x7F,0x3B,0xE3,0x97,0x4F,0xE7,0xAB,0x5F,0xEF,0xBF,0x77,0xF7,0xD3,0x8B,
	0xA7,0x7B,0x3B,0xB7,0x9B,0x37,0xC7,0xC3,0x37,0xE7,0xE3,0x57,0x00,0xFF,0x00,0xAB,
	0xE7,0xFF,0xD7,0xFF,0xFF,0x67,0x00,0x00,0x8B,0x00,0x00,0xB3,0x00,0x00,0xD7,0x00,
	0x00,0xFF,0x00,0x00,0xFF,0xF3,0x93,0xFF,0xF7,0xC7,0xFF,0xFF,0xFF,0x9F,0x5B,0x53,
};

byte *W_GetTexture(char *name, int matchwidth, int matchheight)
{
	int i, c, datasize;
	short colorcount;
	FILE *file;
	struct
	{
		char name[16];
		int width;
		int height;
		int ofs[4];
	} t;
	byte pal[256][3], *indata, *outdata, *data;
	for (i = 0;i < TEXWAD_MAXIMAGES;i++)
	{
		if (texwadlump[i].name[0])
		{
			if (!strcmp(name, texwadlump[i].name)) // found it
			{
				file = texwadlump[i].file;
				if (fseek(file, texwadlump[i].position, SEEK_SET))
				{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				if (fread(&t, sizeof(t), 1, file) != 1)
				{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				image_width = LittleLong(t.width);
				image_height = LittleLong(t.height);
				if (matchwidth && image_width != matchwidth)
					continue;
				if (matchheight && image_height != matchheight)
					continue;
				if (image_width & 15 || image_height & 15)
				{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				// allocate space for expanded image,
				// and load incoming image into upper area (overwritten as it expands)
				if (!(data = outdata = qmalloc(image_width*image_height*4)))
				{Con_Printf("W_GetTexture: out of memory");return FALSE;}
				indata = outdata + image_width*image_height*3;
				datasize = image_width*image_height*85/64;
				// read the image data
				if (fseek(file, texwadlump[i].position + sizeof(t), SEEK_SET))
				{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				if (fread(indata, 1, image_width*image_height, file) != image_width*image_height)
				{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				// read the number of colors used (always 256)
				if (fseek(file, texwadlump[i].position + sizeof(t) + datasize, SEEK_SET))
				{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				if (fread(&colorcount, 2, 1, file) != 1)
				{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				if (texwadlump[i].size > (datasize + sizeof(t)))
				{
					colorcount = LittleShort(colorcount);
					// sanity checking
					if (colorcount < 0) colorcount = 0;
					if (colorcount > 256) colorcount = 256;
					// read the palette
	//				fseek(file, texwadlump[i].position + sizeof(t) + datasize + 2, SEEK_SET);
					if (fread(&pal, 3, colorcount, file) != colorcount)
					{Con_Printf("W_GetTexture: corrupt WAD3 file");return FALSE;}
				}
				else
					memcpy(&pal, hlpalette, 768);
				// expand the image to 32bit RGBA
				for (i = 0;i < image_width*image_height;i++)
				{
					c = *indata++;
					if (pal[c][0] == 255 && pal[c][1] == 0 && pal[c][2] == 0) // HL transparent color (pure blue)
						outdata[0] = outdata[1] = outdata[2] = outdata[3] = 0;
					else
					{
						outdata[0] = pal[c][0];
						outdata[1] = pal[c][1];
						outdata[2] = pal[c][2];
						outdata[3] = 255;
					}
					outdata += 4;
				}
				return data;
			}
		}
		else
			break;
	}
	image_width = image_height = 0;
	return NULL;
}
