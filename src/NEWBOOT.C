/*
    Copyright (C) 1998  Richard L. Dubs

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <dos.h>
#include <bios.h>
#include <mem.h>

#define	FILELENGTH	2000
#define bootfile	"bootnew.bin"

 char buffer[FILELENGTH];

   FILE *fd;
   int result, *result1;
   char  buffer1[512];
   int cylinder, head, sector;

void main(void)  {


	if ((fd = fopen(bootfile,"rb")) == NULL) {
	  printf("fopen boot file");
	  exit();
       }

	if (fread(buffer,sizeof(char),FILELENGTH,fd) == 0) {
	  printf("fread boot file");
	  exit();
       }
	(void)fclose(fd);       /* close boot file */


   cylinder = 0;
   head = 1;
   sector = 1;

   result = biosdisk(2, 0x81, head, cylinder, sector, 1, buffer1);

   result1 = memcpy(buffer1, buffer+0x20, 0x105);

result = biosdisk(3, 0x81, head, cylinder, sector, 1, buffer1);
}




