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

#include <bios.h>
#include <stdio.h>

void dump_bytes(char *bytes, int count);

void main(void)
{
   int result;
   char buffer2[512], buffer1[512];
   int cylinder2, head2, sector2;
   int cylinder1, head1, sector1;

   
   cylinder1 = 0;
   head1 = 1;
   sector1 = 1;

   cylinder2 = 0;
   head2 = 1;
   sector2 = 1;


   result = biosdisk(2, 0x81, head1, cylinder1, sector1, 1, buffer1);

   buffer1[0x106] = 0x0e;
   buffer1[0x107] = 0x50;
   buffer1[0x108] = 0x05;
   buffer1[0x109] = 0x00;
   buffer1[0x10a] = 0x1c;
   buffer1[0x10b] = 0x60;
   buffer1[0x10c] = 0x05;
   buffer1[0x10d] = 0x00;
   
   dump_bytes(buffer1, 512);

result = biosdisk(3, 0x81, head2, cylinder2, sector2, 1, buffer1);
}


void
dump_bytes(char *bytes, int count)
{
	int n;
	char buf[16];
	int address;
	void fmtline();

	address = 0;
	while(count){
		if (count > 16) n = 16;
		else n = count;
		fmtline(address,bytes,n);
		address += n;
		count -= n;
		bytes += n;
	}
}
/* Print a buffer up to 16 bytes long in formatted hex with ascii
 * translation, e.g.,
 * 0000: 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f  0123456789:;<=>?
 */
void
fmtline(addr,buf,len)
int addr;
char *buf;
int len;
{
	char line[80];
	register char *aptr,*cptr;
	unsigned register char c;
	void ctohex();

	memset(line,' ',sizeof(line));
	ctohex(line,addr >> 8);
	ctohex(line+2,addr & 0xff);
	aptr = &line[6];
	cptr = &line[55];
	while(len-- != 0){
		c = *buf++;
		ctohex(aptr,c);
		aptr += 3;
		c &= 0x7f;
		*cptr++ = isprint(c) ? c : '.';
	}
	*cptr++ = '\n';
	fwrite(line,1,(unsigned)(cptr-line),stdout);
}
/* Convert byte to two ascii-hex characters */
static
void
ctohex(buf,c)
register char *buf;
register int c;
{
	static char hex[] = "0123456789abcdef";

	*buf++ = hex[c >> 4];
	*buf = hex[c & 0xf];
}





