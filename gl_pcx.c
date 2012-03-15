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

extern int image_width;
extern int image_height;

/*
===========
PCX Loading
===========
*/

typedef struct
{
	char		manufacturer;
	char		version;
	char		encoding;
	char		bits_per_pixel;
	unsigned short	xmin,ymin,xmax,ymax;
	unsigned short	hres,vres;
	unsigned char	palette[48];
	char		reserved;
	char		color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char		filler[58];
	unsigned 	data;			// unbounded
} pcx_t;

/*
========
LoadPCX
========
*/
byte* LoadPCX (vfsfile_t *f)
{
	pcx_t	*pcx, pcxbuf;
	byte	palette[768];
	byte	*pix, *image_rgba;
	int		x, y;
	int		dataByte, runLength;
	int		count;

//
// parse the PCX file
//
	VFS_READ (f, &pcxbuf, sizeof(pcxbuf));

	pcx = &pcxbuf;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax > 320
		|| pcx->ymax > 256)
	{
		Con_Printf ("Bad pcx file\n");
		VFS_CLOSE(f);
		return NULL;
	}

	// seek to palette
	VFS_SEEK(f, VFS_GETLEN(f)-768);
	VFS_READ(f, palette, 768);


	VFS_SEEK(f, sizeof(pcxbuf) - 4);

	count = (pcx->xmax+1) * (pcx->ymax+1);
	image_rgba = malloc( count * 4);

	for (y=0 ; y<=pcx->ymax ; y++)
	{
		pix = image_rgba + 4*y*(pcx->xmax+1);
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = VFS_GETC(f);

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = VFS_GETC(f);
			}
			else
				runLength = 1;

			while(runLength-- > 0)
			{
				pix[0] = palette[dataByte*3];
				pix[1] = palette[dataByte*3+1];
				pix[2] = palette[dataByte*3+2];
				pix[3] = 255;
				pix += 4;
				x++;
			}
		}
	}
	VFS_CLOSE(f);
	image_width = pcx->xmax+1;
	image_height = pcx->ymax+1;
	return image_rgba;
}