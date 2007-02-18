
#include "quakedef.h"
#include "image.h"
#include "jpeg.h"
#include "image_png.h"
#include "r_shadow.h"

int		image_width;
int		image_height;

#if 1
// written by LordHavoc in a readable way, optimized by Vic, further optimized by LordHavoc (the non-special index case), readable version preserved below this
void Image_CopyMux(unsigned char *outpixels, const unsigned char *inpixels, int inputwidth, int inputheight, qboolean inputflipx, qboolean inputflipy, qboolean inputflipdiagonal, int numoutputcomponents, int numinputcomponents, int *outputinputcomponentindices)
{
	int index, c, x, y;
	const unsigned char *in, *line;
	int row_inc = (inputflipy ? -inputwidth : inputwidth) * numinputcomponents, col_inc = (inputflipx ? -1 : 1) * numinputcomponents;
	int row_ofs = (inputflipy ? (inputheight - 1) * inputwidth * numinputcomponents : 0), col_ofs = (inputflipx ? (inputwidth - 1) * numinputcomponents : 0);

	for (c = 0; c < numoutputcomponents; c++)
		if (outputinputcomponentindices[c] & 0x80000000)
			break;
	if (c < numoutputcomponents)
	{
		// special indices used
		if (inputflipdiagonal)
		{
			for (x = 0, line = inpixels + col_ofs; x < inputwidth; x++, line += col_inc)
				for (y = 0, in = line + row_ofs; y < inputheight; y++, in += row_inc, outpixels += numinputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = ((index = outputinputcomponentindices[c]) & 0x80000000) ? index : in[index];
		}
		else
		{
			for (y = 0, line = inpixels + row_ofs; y < inputheight; y++, line += row_inc)
				for (x = 0, in = line + col_ofs; x < inputwidth; x++, in += col_inc, outpixels += numinputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = ((index = outputinputcomponentindices[c]) & 0x80000000) ? index : in[index];
		}
	}
	else
	{
		// special indices not used
		if (inputflipdiagonal)
		{
			for (x = 0, line = inpixels + col_ofs; x < inputwidth; x++, line += col_inc)
				for (y = 0, in = line + row_ofs; y < inputheight; y++, in += row_inc, outpixels += numinputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = in[outputinputcomponentindices[c]];
		}
		else
		{
			for (y = 0, line = inpixels + row_ofs; y < inputheight; y++, line += row_inc)
				for (x = 0, in = line + col_ofs; x < inputwidth; x++, in += col_inc, outpixels += numinputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = in[outputinputcomponentindices[c]];
		}
	}
}
#else
// intentionally readable version
void Image_CopyMux(unsigned char *outpixels, const unsigned char *inpixels, int inputwidth, int inputheight, qboolean inputflipx, qboolean inputflipy, qboolean inputflipdiagonal, int numoutputcomponents, int numinputcomponents, int *outputinputcomponentindices)
{
	int index, c, x, y;
	const unsigned char *in, *inrow, *incolumn;
	if (inputflipdiagonal)
	{
		for (x = 0;x < inputwidth;x++)
		{
			for (y = 0;y < inputheight;y++)
			{
				in = inpixels + ((inputflipy ? inputheight - 1 - y : y) * inputwidth + (inputflipx ? inputwidth - 1 - x : x)) * numinputcomponents;
				for (c = 0;c < numoutputcomponents;c++)
				{
					index = outputinputcomponentindices[c];
					if (index & 0x80000000)
						*outpixels++ = index;
					else
						*outpixels++ = in[index];
				}
			}
		}
	}
	else
	{
		for (y = 0;y < inputheight;y++)
		{
			for (x = 0;x < inputwidth;x++)
			{
				in = inpixels + ((inputflipy ? inputheight - 1 - y : y) * inputwidth + (inputflipx ? inputwidth - 1 - x : x)) * numinputcomponents;
				for (c = 0;c < numoutputcomponents;c++)
				{
					index = outputinputcomponentindices[c];
					if (index & 0x80000000)
						*outpixels++ = index;
					else
						*outpixels++ = in[index];
				}
			}
		}
	}
}
#endif

void Image_GammaRemapRGB(const unsigned char *in, unsigned char *out, int pixels, const unsigned char *gammar, const unsigned char *gammag, const unsigned char *gammab)
{
	while (pixels--)
	{
		out[0] = gammar[in[0]];
		out[1] = gammag[in[1]];
		out[2] = gammab[in[2]];
		in += 3;
		out += 3;
	}
}

// note: pal must be 32bit color
void Image_Copy8bitRGBA(const unsigned char *in, unsigned char *out, int pixels, const unsigned int *pal)
{
	int *iout = (int *)out;
	while (pixels >= 8)
	{
		iout[0] = pal[in[0]];
		iout[1] = pal[in[1]];
		iout[2] = pal[in[2]];
		iout[3] = pal[in[3]];
		iout[4] = pal[in[4]];
		iout[5] = pal[in[5]];
		iout[6] = pal[in[6]];
		iout[7] = pal[in[7]];
		in += 8;
		iout += 8;
		pixels -= 8;
	}
	if (pixels & 4)
	{
		iout[0] = pal[in[0]];
		iout[1] = pal[in[1]];
		iout[2] = pal[in[2]];
		iout[3] = pal[in[3]];
		in += 4;
		iout += 4;
	}
	if (pixels & 2)
	{
		iout[0] = pal[in[0]];
		iout[1] = pal[in[1]];
		in += 2;
		iout += 2;
	}
	if (pixels & 1)
		iout[0] = pal[in[0]];
}

/*
=================================================================

  PCX Loading

=================================================================
*/

typedef struct pcx_s
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
} pcx_t;

/*
============
LoadPCX
============
*/
unsigned char* LoadPCX (const unsigned char *f, int filesize, int matchwidth, int matchheight)
{
	pcx_t pcx;
	unsigned char *a, *b, *image_rgba, *pbuf;
	const unsigned char *palette, *fin, *enddata;
	int x, y, x2, dataByte;

	if (filesize < (int)sizeof(pcx) + 768)
	{
		Con_Print("Bad pcx file\n");
		return NULL;
	}

	fin = f;

	memcpy(&pcx, fin, sizeof(pcx));
	fin += sizeof(pcx);

	// LordHavoc: big-endian support ported from QF newtree
	pcx.xmax = LittleShort (pcx.xmax);
	pcx.xmin = LittleShort (pcx.xmin);
	pcx.ymax = LittleShort (pcx.ymax);
	pcx.ymin = LittleShort (pcx.ymin);
	pcx.hres = LittleShort (pcx.hres);
	pcx.vres = LittleShort (pcx.vres);
	pcx.bytes_per_line = LittleShort (pcx.bytes_per_line);
	pcx.palette_type = LittleShort (pcx.palette_type);

	image_width = pcx.xmax + 1 - pcx.xmin;
	image_height = pcx.ymax + 1 - pcx.ymin;
	if (pcx.manufacturer != 0x0a || pcx.version != 5 || pcx.encoding != 1 || pcx.bits_per_pixel != 8 || image_width > 4096 || image_height > 4096 || image_width <= 0 || image_height <= 0)
	{
		Con_Print("Bad pcx file\n");
		return NULL;
	}
	if ((matchwidth && image_width != matchwidth) || (matchheight && image_height != matchheight))
		return NULL;

	palette = f + filesize - 768;

	image_rgba = (unsigned char *)Mem_Alloc(tempmempool, image_width*image_height*4);
	if (!image_rgba)
	{
		Con_Printf("LoadPCX: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}
	pbuf = image_rgba + image_width*image_height*3;
	enddata = palette;

	for (y = 0;y < image_height && fin < enddata;y++)
	{
		a = pbuf + y * image_width;
		for (x = 0;x < image_width && fin < enddata;)
		{
			dataByte = *fin++;
			if(dataByte >= 0xC0)
			{
				if (fin >= enddata)
					break;
				x2 = x + (dataByte & 0x3F);
				dataByte = *fin++;
				if (x2 > image_width)
					x2 = image_width; // technically an error
				while(x < x2)
					a[x++] = dataByte;
			}
			else
				a[x++] = dataByte;
		}
		fin += pcx.bytes_per_line - image_width; // the number of bytes per line is always forced to an even number
		while(x < image_width)
			a[x++] = 0;
	}

	a = image_rgba;
	b = pbuf;

	for(x = 0;x < image_width*image_height;x++)
	{
		y = *b++ * 3;
		*a++ = palette[y];
		*a++ = palette[y+1];
		*a++ = palette[y+2];
		*a++ = 255;
	}

	return image_rgba;
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader
{
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
}
TargaHeader;

void PrintTargaHeader(TargaHeader *t)
{
	Con_Printf("TargaHeader:\nuint8 id_length = %i;\nuint8 colormap_type = %i;\nuint8 image_type = %i;\nuint16 colormap_index = %i;\nuint16 colormap_length = %i;\nuint8 colormap_size = %i;\nuint16 x_origin = %i;\nuint16 y_origin = %i;\nuint16 width = %i;\nuint16 height = %i;\nuint8 pixel_size = %i;\nuint8 attributes = %i;\n", t->id_length, t->colormap_type, t->image_type, t->colormap_index, t->colormap_length, t->colormap_size, t->x_origin, t->y_origin, t->width, t->height, t->pixel_size, t->attributes);
}

/*
=============
LoadTGA
=============
*/
unsigned char *LoadTGA (const unsigned char *f, int filesize, int matchwidth, int matchheight)
{
	int x, y, pix_inc, row_inc, red, green, blue, alpha, runlen, alphabits;
	unsigned char *pixbuf, *image_rgba;
	const unsigned char *fin, *enddata;
	unsigned char *p;
	TargaHeader targa_header;
	unsigned char palette[256*4];

	if (filesize < 19)
		return NULL;

	enddata = f + filesize;

	targa_header.id_length = f[0];
	targa_header.colormap_type = f[1];
	targa_header.image_type = f[2];

	targa_header.colormap_index = f[3] + f[4] * 256;
	targa_header.colormap_length = f[5] + f[6] * 256;
	targa_header.colormap_size = f[7];
	targa_header.x_origin = f[8] + f[9] * 256;
	targa_header.y_origin = f[10] + f[11] * 256;
	targa_header.width = image_width = f[12] + f[13] * 256;
	targa_header.height = image_height = f[14] + f[15] * 256;
	if (image_width > 4096 || image_height > 4096 || image_width <= 0 || image_height <= 0)
	{
		Con_Print("LoadTGA: invalid size\n");
		PrintTargaHeader(&targa_header);
		return NULL;
	}
	if ((matchwidth && image_width != matchwidth) || (matchheight && image_height != matchheight))
		return NULL;
	targa_header.pixel_size = f[16];
	targa_header.attributes = f[17];

	// advance to end of header
	fin = f + 18;

	// skip TARGA image comment (usually 0 bytes)
	fin += targa_header.id_length;

	// read/skip the colormap if present (note: according to the TARGA spec it
	// can be present even on truecolor or greyscale images, just not used by
	// the image data)
	if (targa_header.colormap_type)
	{
		if (targa_header.colormap_length > 256)
		{
			Con_Print("LoadTGA: only up to 256 colormap_length supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		if (targa_header.colormap_index)
		{
			Con_Print("LoadTGA: colormap_index not supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		if (targa_header.colormap_size == 24)
		{
			for (x = 0;x < targa_header.colormap_length;x++)
			{
				palette[x*4+2] = *fin++;
				palette[x*4+1] = *fin++;
				palette[x*4+0] = *fin++;
				palette[x*4+3] = 255;
			}
		}
		else if (targa_header.colormap_size == 32)
		{
			for (x = 0;x < targa_header.colormap_length;x++)
			{
				palette[x*4+2] = *fin++;
				palette[x*4+1] = *fin++;
				palette[x*4+0] = *fin++;
				palette[x*4+3] = *fin++;
			}
		}
		else
		{
			Con_Print("LoadTGA: Only 32 and 24 bit colormap_size supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
	}

	// check our pixel_size restrictions according to image_type
	switch (targa_header.image_type & ~8)
	{
	case 2:
		if (targa_header.pixel_size != 24 && targa_header.pixel_size != 32)
		{
			Con_Print("LoadTGA: only 24bit and 32bit pixel sizes supported for type 2 and type 10 images\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		break;
	case 3:
		// set up a palette to make the loader easier
		for (x = 0;x < 256;x++)
		{
			palette[x*4+2] = x;
			palette[x*4+1] = x;
			palette[x*4+0] = x;
			palette[x*4+3] = 255;
		}
		// fall through to colormap case
	case 1:
		if (targa_header.pixel_size != 8)
		{
			Con_Print("LoadTGA: only 8bit pixel size for type 1, 3, 9, and 11 images supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		break;
	default:
		Con_Printf("LoadTGA: Only type 1, 2, 3, 9, 10, and 11 targa RGB images supported, image_type = %i\n", targa_header.image_type);
		PrintTargaHeader(&targa_header);
		return NULL;
	}

	if (targa_header.attributes & 0x10)
	{
		Con_Print("LoadTGA: origin must be in top left or bottom left, top right and bottom right are not supported\n");
		return NULL;
	}

	// number of attribute bits per pixel, we only support 0 or 8
	alphabits = targa_header.attributes & 0x0F;
	if (alphabits != 8 && alphabits != 0)
	{
		Con_Print("LoadTGA: only 0 or 8 attribute (alpha) bits supported\n");
		return NULL;
	}

	image_rgba = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height * 4);
	if (!image_rgba)
	{
		Con_Printf("LoadTGA: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}

	// If bit 5 of attributes isn't set, the image has been stored from bottom to top
	if ((targa_header.attributes & 0x20) == 0)
	{
		pixbuf = image_rgba + (image_height - 1)*image_width*4;
		row_inc = -image_width*4*2;
	}
	else
	{
		pixbuf = image_rgba;
		row_inc = 0;
	}

	x = 0;
	y = 0;
	red = green = blue = alpha = 255;
	pix_inc = 1;
	if ((targa_header.image_type & ~8) == 2)
		pix_inc = targa_header.pixel_size / 8;
	switch (targa_header.image_type)
	{
	case 1: // colormapped, uncompressed
	case 3: // greyscale, uncompressed
		if (fin + image_width * image_height * pix_inc > enddata)
			break;
		for (y = 0;y < image_height;y++, pixbuf += row_inc)
		{
			for (x = 0;x < image_width;x++)
			{
				p = palette + *fin++ * 4;
				*pixbuf++ = p[0];
				*pixbuf++ = p[1];
				*pixbuf++ = p[2];
				*pixbuf++ = p[3];
			}
		}
		break;
	case 2:
		// BGR or BGRA, uncompressed
		if (fin + image_width * image_height * pix_inc > enddata)
			break;
		if (targa_header.pixel_size == 32 && alphabits)
		{
			for (y = 0;y < image_height;y++, pixbuf += row_inc)
			{
				for (x = 0;x < image_width;x++, fin += pix_inc)
				{
					*pixbuf++ = fin[2];
					*pixbuf++ = fin[1];
					*pixbuf++ = fin[0];
					*pixbuf++ = fin[3];
				}
			}
		}
		else
		{
			for (y = 0;y < image_height;y++, pixbuf += row_inc)
			{
				for (x = 0;x < image_width;x++, fin += pix_inc)
				{
					*pixbuf++ = fin[2];
					*pixbuf++ = fin[1];
					*pixbuf++ = fin[0];
					*pixbuf++ = 255;
				}
			}
		}
		break;
	case 9: // colormapped, RLE
	case 11: // greyscale, RLE
		for (y = 0;y < image_height;y++, pixbuf += row_inc)
		{
			for (x = 0;x < image_width;)
			{
				if (fin >= enddata)
					break; // error - truncated file
				runlen = *fin++;
				if (runlen & 0x80)
				{
					// RLE - all pixels the same color
					runlen += 1 - 0x80;
					if (fin + pix_inc > enddata)
						break; // error - truncated file
					if (x + runlen > image_width)
						break; // error - line exceeds width
					p = palette + *fin++ * 4;
					red = p[0];
					green = p[1];
					blue = p[2];
					alpha = p[3];
					for (;runlen--;x++)
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alpha;
					}
				}
				else
				{
					// uncompressed - all pixels different color
					runlen++;
					if (fin + pix_inc * runlen > enddata)
						break; // error - truncated file
					if (x + runlen > image_width)
						break; // error - line exceeds width
					for (;runlen--;x++)
					{
						p = palette + *fin++ * 4;
						*pixbuf++ = p[0];
						*pixbuf++ = p[1];
						*pixbuf++ = p[2];
						*pixbuf++ = p[3];
					}
				}
			}
		}
		break;
	case 10:
		// BGR or BGRA, RLE
		if (targa_header.pixel_size == 32 && alphabits)
		{
			for (y = 0;y < image_height;y++, pixbuf += row_inc)
			{
				for (x = 0;x < image_width;)
				{
					if (fin >= enddata)
						break; // error - truncated file
					runlen = *fin++;
					if (runlen & 0x80)
					{
						// RLE - all pixels the same color
						runlen += 1 - 0x80;
						if (fin + pix_inc > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						red = fin[2];
						green = fin[1];
						blue = fin[0];
						alpha = fin[3];
						fin += pix_inc;
						for (;runlen--;x++)
						{
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alpha;
						}
					}
					else
					{
						// uncompressed - all pixels different color
						runlen++;
						if (fin + pix_inc * runlen > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						for (;runlen--;x++, fin += pix_inc)
						{
							*pixbuf++ = fin[2];
							*pixbuf++ = fin[1];
							*pixbuf++ = fin[0];
							*pixbuf++ = fin[3];
						}
					}
				}
			}
		}
		else
		{
			for (y = 0;y < image_height;y++, pixbuf += row_inc)
			{
				for (x = 0;x < image_width;)
				{
					if (fin >= enddata)
						break; // error - truncated file
					runlen = *fin++;
					if (runlen & 0x80)
					{
						// RLE - all pixels the same color
						runlen += 1 - 0x80;
						if (fin + pix_inc > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						red = fin[2];
						green = fin[1];
						blue = fin[0];
						alpha = 255;
						fin += pix_inc;
						for (;runlen--;x++)
						{
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alpha;
						}
					}
					else
					{
						// uncompressed - all pixels different color
						runlen++;
						if (fin + pix_inc * runlen > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						for (;runlen--;x++, fin += pix_inc)
						{
							*pixbuf++ = fin[2];
							*pixbuf++ = fin[1];
							*pixbuf++ = fin[0];
							*pixbuf++ = 255;
						}
					}
				}
			}
		}
		break;
	default:
		// unknown image_type
		break;
	}

	return image_rgba;
}

/*
============
LoadLMP
============
*/
unsigned char *LoadLMP (const unsigned char *f, int filesize, int matchwidth, int matchheight, qboolean loadAs8Bit)
{
	unsigned char *image_buffer;

	if (filesize < 9)
	{
		Con_Print("LoadLMP: invalid LMP file\n");
		return NULL;
	}

	// parse the very complicated header *chuckle*
	image_width = BuffLittleLong(f);
	image_height = BuffLittleLong(f + 4);
	if (image_width > 4096 || image_height > 4096 || image_width <= 0 || image_height <= 0)
	{
		Con_Printf("LoadLMP: invalid size %ix%i\n", image_width, image_height);
		return NULL;
	}
	if ((matchwidth && image_width != matchwidth) || (matchheight && image_height != matchheight))
		return NULL;

	if (filesize < (8 + image_width * image_height))
	{
		Con_Print("LoadLMP: invalid LMP file\n");
		return NULL;
	}

	if (loadAs8Bit)
	{
		image_buffer = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height);
		memcpy(image_buffer, f + 8, image_width * image_height);
	}
	else
	{
		image_buffer = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height * 4);
		Image_Copy8bitRGBA(f + 8, image_buffer, image_width * image_height, palette_transparent);
	}
	return image_buffer;
}


typedef struct q2wal_s
{
	char		name[32];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	char		animname[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} q2wal_t;

unsigned char *LoadWAL (const unsigned char *f, int filesize, int matchwidth, int matchheight)
{
	unsigned char *image_rgba;
	const q2wal_t *inwal = (const q2wal_t *)f;

	if (filesize < (int) sizeof(q2wal_t))
	{
		Con_Print("LoadWAL: invalid WAL file\n");
		return NULL;
	}

	image_width = LittleLong(inwal->width);
	image_height = LittleLong(inwal->height);
	if (image_width > 4096 || image_height > 4096 || image_width <= 0 || image_height <= 0)
	{
		Con_Printf("LoadWAL: invalid size %ix%i\n", image_width, image_height);
		return NULL;
	}
	if ((matchwidth && image_width != matchwidth) || (matchheight && image_height != matchheight))
		return NULL;

	if (filesize < (int) sizeof(q2wal_t) + (int) LittleLong(inwal->offsets[0]) + image_width * image_height)
	{
		Con_Print("LoadWAL: invalid WAL file\n");
		return NULL;
	}

	image_rgba = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height * 4);
	if (!image_rgba)
	{
		Con_Printf("LoadLMP: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}
	Image_Copy8bitRGBA(f + LittleLong(inwal->offsets[0]), image_rgba, image_width * image_height, palette_complete);
	return image_rgba;
}


static void Image_StripImageExtension (const char *in, char *out, size_t size_out)
{
	const char *end, *temp;

	if (size_out == 0)
		return;

	end = in + strlen(in);
	if ((end - in) >= 4)
	{
		temp = end - 4;
		if (strcmp(temp, ".tga") == 0
		 || strcmp(temp, ".pcx") == 0
		 || strcmp(temp, ".lmp") == 0
		 || strcmp(temp, ".png") == 0
		 || strcmp(temp, ".jpg") == 0)
			end = temp;
		while (in < end && size_out > 1)
		{
			*out++ = *in++;
			size_out--;
		}
		*out++ = 0;
	}
	else
		strlcpy(out, in, size_out);
}

typedef struct imageformat_s
{
	const char *formatstring;
	unsigned char *(*loadfunc)(const unsigned char *f, int filesize, int matchwidth, int matchheight);
}
imageformat_t;

// GAME_TENEBRAE only
imageformat_t imageformats_tenebrae[] =
{
	{"override/%s.tga", LoadTGA},
	{"override/%s.png", PNG_LoadImage},
	{"override/%s.jpg", JPEG_LoadImage},
	{"override/%s.pcx", LoadPCX},
	{NULL, NULL}
};

imageformat_t imageformats_nopath[] =
{
	{"override/%s.tga", LoadTGA},
	{"override/%s.png", PNG_LoadImage},
	{"override/%s.jpg", JPEG_LoadImage},
	{"textures/%s.tga", LoadTGA},
	{"textures/%s.png", PNG_LoadImage},
	{"textures/%s.jpg", JPEG_LoadImage},
	{"%s.tga", LoadTGA},
	{"%s.png", PNG_LoadImage},
	{"%s.jpg", JPEG_LoadImage},
	{"%s.pcx", LoadPCX},
	{NULL, NULL}
};

imageformat_t imageformats_textures[] =
{
	{"%s.tga", LoadTGA},
	{"%s.png", PNG_LoadImage},
	{"%s.jpg", JPEG_LoadImage},
	{"%s.pcx", LoadPCX},
	{"%s.wal", LoadWAL},
	{NULL, NULL}
};

imageformat_t imageformats_gfx[] =
{
	{"%s.tga", LoadTGA},
	{"%s.png", PNG_LoadImage},
	{"%s.jpg", JPEG_LoadImage},
	{"%s.pcx", LoadPCX},
	{NULL, NULL}
};

imageformat_t imageformats_other[] =
{
	{"%s.tga", LoadTGA},
	{"%s.png", PNG_LoadImage},
	{"%s.jpg", JPEG_LoadImage},
	{"%s.pcx", LoadPCX},
	{NULL, NULL}
};

unsigned char *loadimagepixels (const char *filename, qboolean complain, int matchwidth, int matchheight)
{
	fs_offset_t filesize;
	imageformat_t *firstformat, *format;
	unsigned char *f, *data = NULL;
	char basename[MAX_QPATH], name[MAX_QPATH], *c;
	if (developer_memorydebug.integer)
		Mem_CheckSentinelsGlobal();
	if (developer_texturelogging.integer)
		Log_Printf("textures.log", "%s\n", filename);
	Image_StripImageExtension(filename, basename, sizeof(basename)); // strip filename extensions to allow replacement by other types
	// replace *'s with #, so commandline utils don't get confused when dealing with the external files
	for (c = basename;*c;c++)
		if (*c == '*')
			*c = '#';
	name[0] = 0;
	if (strchr(basename, '/'))
	{
		int i;
		for (i = 0;i < (int)sizeof(name)-1 && basename[i] != '/';i++)
			name[i] = basename[i];
		name[i] = 0;
	}
	if (gamemode == GAME_TENEBRAE)
		firstformat = imageformats_tenebrae;
	else if (!strcasecmp(name, "textures"))
		firstformat = imageformats_textures;
	else if (!strcasecmp(name, "gfx"))
		firstformat = imageformats_gfx;
	else if (!strchr(name, '/'))
		firstformat = imageformats_nopath;
	else
		firstformat = imageformats_other;
	// now try all the formats in the selected list
	for (format = firstformat;format->formatstring;format++)
	{
		sprintf (name, format->formatstring, basename);
		f = FS_LoadFile(name, tempmempool, true, &filesize);
		if (f)
		{
			data = format->loadfunc(f, filesize, matchwidth, matchheight);
			Mem_Free(f);
			if (data)
			{
				if (developer.integer >= 10)
					Con_Printf("loaded image %s (%dx%d)\n", name, image_width, image_height);
				if (developer_memorydebug.integer)
					Mem_CheckSentinelsGlobal();
				return data;
			}
			else
			{
				if (developer.integer >= 1)
					Con_Printf("Error loading image %s (file loaded but decode failed)\n", name);
			}
		}
	}
	if (complain)
	{
		Con_Printf("Couldn't load %s using ", filename);
		for (format = firstformat;format->formatstring;format++)
		{
			sprintf (name, format->formatstring, basename);
			Con_Printf(format == firstformat ? "\"%s\"" : (format[1].formatstring ? ", \"%s\"" : " or \"%s\".\n"), format->formatstring);
		}
	}
	if (developer_memorydebug.integer)
		Mem_CheckSentinelsGlobal();
	return NULL;
}

rtexture_t *loadtextureimage (rtexturepool_t *pool, const char *filename, int matchwidth, int matchheight, qboolean complain, int flags)
{
	unsigned char *data;
	rtexture_t *rt;
	if (!(data = loadimagepixels (filename, complain, matchwidth, matchheight)))
		return 0;
	rt = R_LoadTexture2D(pool, filename, image_width, image_height, data, TEXTYPE_RGBA, flags, NULL);
	Mem_Free(data);
	return rt;
}

qboolean Image_WriteTGARGB_preflipped (const char *filename, int width, int height, const unsigned char *data, unsigned char *buffer)
{
	qboolean ret;
	unsigned char *out;
	const unsigned char *in, *end;

	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = (width >> 0) & 0xFF;
	buffer[13] = (width >> 8) & 0xFF;
	buffer[14] = (height >> 0) & 0xFF;
	buffer[15] = (height >> 8) & 0xFF;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr
	in = data;
	out = buffer + 18;
	end = in + width*height*3;
	for (;in < end;in += 3)
	{
		*out++ = in[2];
		*out++ = in[1];
		*out++ = in[0];
	}
	ret = FS_WriteFile (filename, buffer, width*height*3 + 18 );

	return ret;
}

void Image_WriteTGARGBA (const char *filename, int width, int height, const unsigned char *data)
{
	int y;
	unsigned char *buffer, *out;
	const unsigned char *in, *end;

	buffer = (unsigned char *)Mem_Alloc(tempmempool, width*height*4 + 18);

	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = (width >> 0) & 0xFF;
	buffer[13] = (width >> 8) & 0xFF;
	buffer[14] = (height >> 0) & 0xFF;
	buffer[15] = (height >> 8) & 0xFF;
	buffer[16] = 32;	// pixel size
	buffer[17] = 8; // 8 bits of alpha

	// swap rgba to bgra and flip upside down
	out = buffer + 18;
	for (y = height - 1;y >= 0;y--)
	{
		in = data + y * width * 4;
		end = in + width * 4;
		for (;in < end;in += 4)
		{
			*out++ = in[2];
			*out++ = in[1];
			*out++ = in[0];
			*out++ = in[3];
		}
	}
	FS_WriteFile (filename, buffer, width*height*4 + 18 );

	Mem_Free(buffer);
}

static void Image_Resample32LerpLine (const unsigned char *in, unsigned char *out, int inwidth, int outwidth)
{
	int		j, xi, oldx = 0, f, fstep, endx, lerp;
	fstep = (int) (inwidth*65536.0f/outwidth);
	endx = (inwidth-1);
	for (j = 0,f = 0;j < outwidth;j++, f += fstep)
	{
		xi = f >> 16;
		if (xi != oldx)
		{
			in += (xi - oldx) * 4;
			oldx = xi;
		}
		if (xi < endx)
		{
			lerp = f & 0xFFFF;
			*out++ = (unsigned char) ((((in[4] - in[0]) * lerp) >> 16) + in[0]);
			*out++ = (unsigned char) ((((in[5] - in[1]) * lerp) >> 16) + in[1]);
			*out++ = (unsigned char) ((((in[6] - in[2]) * lerp) >> 16) + in[2]);
			*out++ = (unsigned char) ((((in[7] - in[3]) * lerp) >> 16) + in[3]);
		}
		else // last pixel of the line has no pixel to lerp to
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = in[2];
			*out++ = in[3];
		}
	}
}

static void Image_Resample24LerpLine (const unsigned char *in, unsigned char *out, int inwidth, int outwidth)
{
	int		j, xi, oldx = 0, f, fstep, endx, lerp;
	fstep = (int) (inwidth*65536.0f/outwidth);
	endx = (inwidth-1);
	for (j = 0,f = 0;j < outwidth;j++, f += fstep)
	{
		xi = f >> 16;
		if (xi != oldx)
		{
			in += (xi - oldx) * 3;
			oldx = xi;
		}
		if (xi < endx)
		{
			lerp = f & 0xFFFF;
			*out++ = (unsigned char) ((((in[3] - in[0]) * lerp) >> 16) + in[0]);
			*out++ = (unsigned char) ((((in[4] - in[1]) * lerp) >> 16) + in[1]);
			*out++ = (unsigned char) ((((in[5] - in[2]) * lerp) >> 16) + in[2]);
		}
		else // last pixel of the line has no pixel to lerp to
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = in[2];
		}
	}
}

#define LERPBYTE(i) r = resamplerow1[i];out[i] = (unsigned char) ((((resamplerow2[i] - r) * lerp) >> 16) + r)
void Image_Resample32Lerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j, r, yi, oldy, f, fstep, lerp, endy = (inheight-1), inwidth4 = inwidth*4, outwidth4 = outwidth*4;
	unsigned char *out;
	const unsigned char *inrow;
	unsigned char *resamplerow1;
	unsigned char *resamplerow2;
	out = (unsigned char *)outdata;
	fstep = (int) (inheight*65536.0f/outheight);

	resamplerow1 = (unsigned char *)Mem_Alloc(tempmempool, outwidth*4*2);
	resamplerow2 = resamplerow1 + outwidth*4;

	inrow = (const unsigned char *)indata;
	oldy = 0;
	Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
	Image_Resample32LerpLine (inrow + inwidth4, resamplerow2, inwidth, outwidth);
	for (i = 0, f = 0;i < outheight;i++,f += fstep)
	{
		yi = f >> 16;
		if (yi < endy)
		{
			lerp = f & 0xFFFF;
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
				Image_Resample32LerpLine (inrow + inwidth4, resamplerow2, inwidth, outwidth);
				oldy = yi;
			}
			j = outwidth - 4;
			while(j >= 0)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				LERPBYTE( 6);
				LERPBYTE( 7);
				LERPBYTE( 8);
				LERPBYTE( 9);
				LERPBYTE(10);
				LERPBYTE(11);
				LERPBYTE(12);
				LERPBYTE(13);
				LERPBYTE(14);
				LERPBYTE(15);
				out += 16;
				resamplerow1 += 16;
				resamplerow2 += 16;
				j -= 4;
			}
			if (j & 2)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				LERPBYTE( 6);
				LERPBYTE( 7);
				out += 8;
				resamplerow1 += 8;
				resamplerow2 += 8;
			}
			if (j & 1)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				out += 4;
				resamplerow1 += 4;
				resamplerow2 += 4;
			}
			resamplerow1 -= outwidth4;
			resamplerow2 -= outwidth4;
		}
		else
		{
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
				oldy = yi;
			}
			memcpy(out, resamplerow1, outwidth4);
		}
	}

	Mem_Free(resamplerow1);
	resamplerow1 = NULL;
	resamplerow2 = NULL;
}

void Image_Resample32Nolerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j;
	unsigned frac, fracstep;
	// relies on int being 4 bytes
	int *inrow, *out;
	out = (int *)outdata;

	fracstep = inwidth*0x10000/outwidth;
	for (i = 0;i < outheight;i++)
	{
		inrow = (int *)indata + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		j = outwidth - 4;
		while (j >= 0)
		{
			out[0] = inrow[frac >> 16];frac += fracstep;
			out[1] = inrow[frac >> 16];frac += fracstep;
			out[2] = inrow[frac >> 16];frac += fracstep;
			out[3] = inrow[frac >> 16];frac += fracstep;
			out += 4;
			j -= 4;
		}
		if (j & 2)
		{
			out[0] = inrow[frac >> 16];frac += fracstep;
			out[1] = inrow[frac >> 16];frac += fracstep;
			out += 2;
		}
		if (j & 1)
		{
			out[0] = inrow[frac >> 16];frac += fracstep;
			out += 1;
		}
	}
}

void Image_Resample24Lerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j, r, yi, oldy, f, fstep, lerp, endy = (inheight-1), inwidth3 = inwidth * 3, outwidth3 = outwidth * 3;
	unsigned char *out;
	const unsigned char *inrow;
	unsigned char *resamplerow1;
	unsigned char *resamplerow2;
	out = (unsigned char *)outdata;
	fstep = (int) (inheight*65536.0f/outheight);

	resamplerow1 = (unsigned char *)Mem_Alloc(tempmempool, outwidth*3*2);
	resamplerow2 = resamplerow1 + outwidth*3;

	inrow = (const unsigned char *)indata;
	oldy = 0;
	Image_Resample24LerpLine (inrow, resamplerow1, inwidth, outwidth);
	Image_Resample24LerpLine (inrow + inwidth3, resamplerow2, inwidth, outwidth);
	for (i = 0, f = 0;i < outheight;i++,f += fstep)
	{
		yi = f >> 16;
		if (yi < endy)
		{
			lerp = f & 0xFFFF;
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth3*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth3);
				else
					Image_Resample24LerpLine (inrow, resamplerow1, inwidth, outwidth);
				Image_Resample24LerpLine (inrow + inwidth3, resamplerow2, inwidth, outwidth);
				oldy = yi;
			}
			j = outwidth - 4;
			while(j >= 0)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				LERPBYTE( 6);
				LERPBYTE( 7);
				LERPBYTE( 8);
				LERPBYTE( 9);
				LERPBYTE(10);
				LERPBYTE(11);
				out += 12;
				resamplerow1 += 12;
				resamplerow2 += 12;
				j -= 4;
			}
			if (j & 2)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				out += 6;
				resamplerow1 += 6;
				resamplerow2 += 6;
			}
			if (j & 1)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				out += 3;
				resamplerow1 += 3;
				resamplerow2 += 3;
			}
			resamplerow1 -= outwidth3;
			resamplerow2 -= outwidth3;
		}
		else
		{
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth3*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth3);
				else
					Image_Resample24LerpLine (inrow, resamplerow1, inwidth, outwidth);
				oldy = yi;
			}
			memcpy(out, resamplerow1, outwidth3);
		}
	}
	Mem_Free(resamplerow1);
	resamplerow1 = NULL;
	resamplerow2 = NULL;
}

void Image_Resample24Nolerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j, f, inwidth3 = inwidth * 3;
	unsigned frac, fracstep;
	unsigned char *inrow, *out;
	out = (unsigned char *)outdata;

	fracstep = inwidth*0x10000/outwidth;
	for (i = 0;i < outheight;i++)
	{
		inrow = (unsigned char *)indata + inwidth3*(i*inheight/outheight);
		frac = fracstep >> 1;
		j = outwidth - 4;
		while (j >= 0)
		{
			f = (frac >> 16)*3;*out++ = inrow[f+0];*out++ = inrow[f+1];*out++ = inrow[f+2];frac += fracstep;
			f = (frac >> 16)*3;*out++ = inrow[f+0];*out++ = inrow[f+1];*out++ = inrow[f+2];frac += fracstep;
			f = (frac >> 16)*3;*out++ = inrow[f+0];*out++ = inrow[f+1];*out++ = inrow[f+2];frac += fracstep;
			f = (frac >> 16)*3;*out++ = inrow[f+0];*out++ = inrow[f+1];*out++ = inrow[f+2];frac += fracstep;
			j -= 4;
		}
		if (j & 2)
		{
			f = (frac >> 16)*3;*out++ = inrow[f+0];*out++ = inrow[f+1];*out++ = inrow[f+2];frac += fracstep;
			f = (frac >> 16)*3;*out++ = inrow[f+0];*out++ = inrow[f+1];*out++ = inrow[f+2];frac += fracstep;
			out += 2;
		}
		if (j & 1)
		{
			f = (frac >> 16)*3;*out++ = inrow[f+0];*out++ = inrow[f+1];*out++ = inrow[f+2];frac += fracstep;
			out += 1;
		}
	}
}

/*
================
Image_Resample
================
*/
void Image_Resample (const void *indata, int inwidth, int inheight, int indepth, void *outdata, int outwidth, int outheight, int outdepth, int bytesperpixel, int quality)
{
	if (indepth != 1 || outdepth != 1)
	{
		Con_Printf ("Image_Resample: 3D resampling not supported\n");
		return;
	}
	if (bytesperpixel == 4)
	{
		if (quality)
			Image_Resample32Lerp(indata, inwidth, inheight, outdata, outwidth, outheight);
		else
			Image_Resample32Nolerp(indata, inwidth, inheight, outdata, outwidth, outheight);
	}
	else if (bytesperpixel == 3)
	{
		if (quality)
			Image_Resample24Lerp(indata, inwidth, inheight, outdata, outwidth, outheight);
		else
			Image_Resample24Nolerp(indata, inwidth, inheight, outdata, outwidth, outheight);
	}
	else
		Con_Printf ("Image_Resample: unsupported bytesperpixel %i\n", bytesperpixel);
}

// in can be the same as out
void Image_MipReduce(const unsigned char *in, unsigned char *out, int *width, int *height, int *depth, int destwidth, int destheight, int destdepth, int bytesperpixel)
{
	const unsigned char *inrow;
	int x, y, nextrow;
	if (*depth != 1 || destdepth != 1)
	{
		Con_Printf ("Image_Resample: 3D resampling not supported\n");
		if (*width > destwidth)
			*width >>= 1;
		if (*height > destheight)
			*height >>= 1;
		if (*depth > destdepth)
			*depth >>= 1;
		return;
	}
	// note: if given odd width/height this discards the last row/column of
	// pixels, rather than doing a proper box-filter scale down
	inrow = in;
	nextrow = *width * bytesperpixel;
	if (*width > destwidth)
	{
		*width >>= 1;
		if (*height > destheight)
		{
			// reduce both
			*height >>= 1;
			if (bytesperpixel == 4)
			{
				for (y = 0;y < *height;y++, inrow += nextrow * 2)
				{
					for (in = inrow, x = 0;x < *width;x++)
					{
						out[0] = (unsigned char) ((in[0] + in[4] + in[nextrow  ] + in[nextrow+4]) >> 2);
						out[1] = (unsigned char) ((in[1] + in[5] + in[nextrow+1] + in[nextrow+5]) >> 2);
						out[2] = (unsigned char) ((in[2] + in[6] + in[nextrow+2] + in[nextrow+6]) >> 2);
						out[3] = (unsigned char) ((in[3] + in[7] + in[nextrow+3] + in[nextrow+7]) >> 2);
						out += 4;
						in += 8;
					}
				}
			}
			else if (bytesperpixel == 3)
			{
				for (y = 0;y < *height;y++, inrow += nextrow * 2)
				{
					for (in = inrow, x = 0;x < *width;x++)
					{
						out[0] = (unsigned char) ((in[0] + in[3] + in[nextrow  ] + in[nextrow+3]) >> 2);
						out[1] = (unsigned char) ((in[1] + in[4] + in[nextrow+1] + in[nextrow+4]) >> 2);
						out[2] = (unsigned char) ((in[2] + in[5] + in[nextrow+2] + in[nextrow+5]) >> 2);
						out += 3;
						in += 6;
					}
				}
			}
			else
				Con_Printf ("Image_MipReduce: unsupported bytesperpixel %i\n", bytesperpixel);
		}
		else
		{
			// reduce width
			if (bytesperpixel == 4)
			{
				for (y = 0;y < *height;y++, inrow += nextrow)
				{
					for (in = inrow, x = 0;x < *width;x++)
					{
						out[0] = (unsigned char) ((in[0] + in[4]) >> 1);
						out[1] = (unsigned char) ((in[1] + in[5]) >> 1);
						out[2] = (unsigned char) ((in[2] + in[6]) >> 1);
						out[3] = (unsigned char) ((in[3] + in[7]) >> 1);
						out += 4;
						in += 8;
					}
				}
			}
			else if (bytesperpixel == 3)
			{
				for (y = 0;y < *height;y++, inrow += nextrow)
				{
					for (in = inrow, x = 0;x < *width;x++)
					{
						out[0] = (unsigned char) ((in[0] + in[3]) >> 1);
						out[1] = (unsigned char) ((in[1] + in[4]) >> 1);
						out[2] = (unsigned char) ((in[2] + in[5]) >> 1);
						out += 3;
						in += 6;
					}
				}
			}
			else
				Con_Printf ("Image_MipReduce: unsupported bytesperpixel %i\n", bytesperpixel);
		}
	}
	else
	{
		if (*height > destheight)
		{
			// reduce height
			*height >>= 1;
			if (bytesperpixel == 4)
			{
				for (y = 0;y < *height;y++, inrow += nextrow * 2)
				{
					for (in = inrow, x = 0;x < *width;x++)
					{
						out[0] = (unsigned char) ((in[0] + in[nextrow  ]) >> 1);
						out[1] = (unsigned char) ((in[1] + in[nextrow+1]) >> 1);
						out[2] = (unsigned char) ((in[2] + in[nextrow+2]) >> 1);
						out[3] = (unsigned char) ((in[3] + in[nextrow+3]) >> 1);
						out += 4;
						in += 4;
					}
				}
			}
			else if (bytesperpixel == 3)
			{
				for (y = 0;y < *height;y++, inrow += nextrow * 2)
				{
					for (in = inrow, x = 0;x < *width;x++)
					{
						out[0] = (unsigned char) ((in[0] + in[nextrow  ]) >> 1);
						out[1] = (unsigned char) ((in[1] + in[nextrow+1]) >> 1);
						out[2] = (unsigned char) ((in[2] + in[nextrow+2]) >> 1);
						out += 3;
						in += 3;
					}
				}
			}
			else
				Con_Printf ("Image_MipReduce: unsupported bytesperpixel %i\n", bytesperpixel);
		}
		else
			Con_Printf ("Image_MipReduce: desired size already achieved\n");
	}
}

void Image_HeightmapToNormalmap(const unsigned char *inpixels, unsigned char *outpixels, int width, int height, int clamp, float bumpscale)
{
	int x, y, x1, x2, y1, y2;
	const unsigned char *b, *row[3];
	int p[5];
	unsigned char *out;
	float iwidth, iheight, ibumpscale, n[3];
	iwidth = 1.0f / width;
	iheight = 1.0f / height;
	ibumpscale = (255.0f * 6.0f) / bumpscale;
	out = outpixels;
	for (y = 0, y1 = height-1;y < height;y1 = y, y++)
	{
		y2 = y + 1;if (y2 >= height) y2 = 0;
		row[0] = inpixels + (y1 * width) * 4;
		row[1] = inpixels + (y  * width) * 4;
		row[2] = inpixels + (y2 * width) * 4;
		for (x = 0, x1 = width-1;x < width;x1 = x, x++)
		{
			x2 = x + 1;if (x2 >= width) x2 = 0;
			// left, right
			b = row[1] + x1 * 4;p[0] = (b[0] + b[1] + b[2]);
			b = row[1] + x2 * 4;p[1] = (b[0] + b[1] + b[2]);
			// above, below
			b = row[0] + x  * 4;p[2] = (b[0] + b[1] + b[2]);
			b = row[2] + x  * 4;p[3] = (b[0] + b[1] + b[2]);
			// center
			b = row[1] + x  * 4;p[4] = (b[0] + b[1] + b[2]);
			// calculate a normal from the slopes
			n[0] = p[0] - p[1];
			n[1] = p[3] - p[2];
			n[2] = ibumpscale;
			VectorNormalize(n);
			// turn it into a dot3 rgb vector texture
			out[0] = (int)(128.0f + n[0] * 127.0f);
			out[1] = (int)(128.0f + n[1] * 127.0f);
			out[2] = (int)(128.0f + n[2] * 127.0f);
			out[3] = (p[4]) / 3;
			out += 4;
		}
	}
}

int image_loadskin(imageskin_t *s, const char *shadername)
{
	int j;
	unsigned char *bumppixels;
	int bumppixels_width, bumppixels_height;
	char name[MAX_QPATH];
	Image_StripImageExtension(shadername, name, sizeof(name));
	memset(s, 0, sizeof(*s));
	s->basepixels = loadimagepixels(name, false, 0, 0);
	if (s->basepixels == NULL)
		return false;
	s->basepixels_width = image_width;
	s->basepixels_height = image_height;

	bumppixels = NULL;bumppixels_width = 0;bumppixels_height = 0;
	for (j = 3;j < s->basepixels_width * s->basepixels_height * 4;j += 4)
		if (s->basepixels[j] < 255)
			break;
	if (j < s->basepixels_width * s->basepixels_height * 4)
	{
		// has transparent pixels
		s->maskpixels = (unsigned char *)Mem_Alloc(loadmodel->mempool, s->basepixels_width * s->basepixels_height * 4);
		s->maskpixels_width = s->basepixels_width;
		s->maskpixels_height = s->basepixels_height;
		memcpy(s->maskpixels, s->basepixels, s->maskpixels_width * s->maskpixels_height * 4);
		for (j = 0;j < s->basepixels_width * s->basepixels_height * 4;j += 4)
		{
			s->maskpixels[j+0] = 255;
			s->maskpixels[j+1] = 255;
			s->maskpixels[j+2] = 255;
		}
	}

	// _luma is supported for tenebrae compatibility
	// (I think it's a very stupid name, but oh well)
	if ((s->glowpixels = loadimagepixels(va("%s_glow", name), false, 0, 0)) != NULL
	 || (s->glowpixels = loadimagepixels(va("%s_luma", name), false, 0, 0)) != NULL)
	{
		s->glowpixels_width = image_width;
		s->glowpixels_height = image_height;
	}
	// _norm is the name used by tenebrae
	// (I don't like the name much)
	if ((s->nmappixels = loadimagepixels(va("%s_norm", name), false, 0, 0)) != NULL)
	{
		s->nmappixels_width = image_width;
		s->nmappixels_height = image_height;
	}
	else if ((bumppixels = loadimagepixels(va("%s_bump", name), false, 0, 0)) != NULL)
	{
		bumppixels_width = image_width;
		bumppixels_height = image_height;
	}
	if ((s->glosspixels = loadimagepixels(va("%s_gloss", name), false, 0, 0)) != NULL)
	{
		s->glosspixels_width = image_width;
		s->glosspixels_height = image_height;
	}
	if ((s->pantspixels = loadimagepixels(va("%s_pants", name), false, 0, 0)) != NULL)
	{
		s->pantspixels_width = image_width;
		s->pantspixels_height = image_height;
	}
	if ((s->shirtpixels = loadimagepixels(va("%s_shirt", name), false, 0, 0)) != NULL)
	{
		s->shirtpixels_width = image_width;
		s->shirtpixels_height = image_height;
	}

	if (s->nmappixels == NULL)
	{
		if (bumppixels != NULL)
		{
			if (r_shadow_bumpscale_bumpmap.value > 0)
			{
				s->nmappixels = (unsigned char *)Mem_Alloc(loadmodel->mempool, bumppixels_width * bumppixels_height * 4);
				s->nmappixels_width = bumppixels_width;
				s->nmappixels_height = bumppixels_height;
				Image_HeightmapToNormalmap(bumppixels, s->nmappixels, s->nmappixels_width, s->nmappixels_height, false, r_shadow_bumpscale_bumpmap.value);
			}
		}
		else
		{
			if (r_shadow_bumpscale_basetexture.value > 0)
			{
				s->nmappixels = (unsigned char *)Mem_Alloc(loadmodel->mempool, s->basepixels_width * s->basepixels_height * 4);
				s->nmappixels_width = s->basepixels_width;
				s->nmappixels_height = s->basepixels_height;
				Image_HeightmapToNormalmap(s->basepixels, s->nmappixels, s->nmappixels_width, s->nmappixels_height, false, r_shadow_bumpscale_basetexture.value);
			}
		}
	}
	if (bumppixels != NULL)
		Mem_Free(bumppixels);
	return true;
}

void image_freeskin(imageskin_t *s)
{
	if (s->basepixels)
		Mem_Free(s->basepixels);
	if (s->maskpixels)
		Mem_Free(s->maskpixels);
	if (s->nmappixels)
		Mem_Free(s->nmappixels);
	if (s->glowpixels)
		Mem_Free(s->glowpixels);
	if (s->glosspixels)
		Mem_Free(s->glosspixels);
	if (s->pantspixels)
		Mem_Free(s->pantspixels);
	if (s->shirtpixels)
		Mem_Free(s->shirtpixels);
	memset(s, 0, sizeof(*s));
}

