/*===========================================================================*

  Prog    : WINI
	    Permite bootear de una particion. Pensado para boot desde
	    menu de DOS para Minix (ShoeLace).

  Compile : bcc -ml wini.c          

  uso     : en config.sys
            shell=wini.exe -h n -w <drive:directory> nro-part 
            
            -h       drive a bootear (0 o 1)
            -w       working directory
            nro-part particion donde bootear
            -d       debug flag

/*===========================================================================*

	Funcionamiento del programa
	
	a) lee informacion del Hard Disk (bootblock) y detecta
	   los tipos de particiones que tiene.
	   
	b) muestra las particiones
	
	c) si se paso como linea de comando un nro de particion
	   es booteada. Si no es valida lo indica y finaliza.

/*===========================================================================*/
/*
   V1.1 - deteccion que no bootea de 2do disco. Cuando se llama al boot
          block en 0:7c00 incorrectamente se pasaba HD1 como parametro sin
          sacarlo de la variable dr en main. Se corrigio y boot ok del 
          2do disco. (13/4/95)   

   V1.0 - primer release del programa, tras muchos beta test.
*/


#define  VERSION "1.1"         /* version */

#include <dos.h>
#include <bios.h>
#include <alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <mem.h>
#include <conio.h>   
#include <fcntl.h>
#include <sys\stat.h>

/*===========================================================================*
/* define's */
/*===========================================================================*/

#define RESETDSK        0       /* function reset disk       */
#define GETSTATUS       1       /* function get drive status */
#define READVBLK        2       /* function read block       */
#define WRITEVBLK       3       /* function write disk block */
#define VERIFYTRK       4       /* function verify disk addr */
#define BLOCK           512     /* buffer size for block     */
#define HD1             0x80    /* hard disk #1              */
#define HD2             0x81    /* hard disk #2              */
#define HDBOOT(d)       disk_io(READVBLK, d, 0, 0, 1, mbr)
#define RDBOOT(d,t,h,s) disk_io(READVBLK, d, t, h, s, buffer)
#define TABLEOFFSET     0x1be   /* offset in boot sector     */
#define VECLEN          2048    /* vector table size         */
#define VECTOR_FILE     "VECTOR.BIN"  /* vector image file   */

/*===========================================================================*
 * Description of entry in partition table
/*===========================================================================*/

struct part_entry {
	unsigned char   bootind; /* boot indicator 0/0x80       */
	unsigned char   start_head;     /* head value for first sector  */
	unsigned char   start_sec;      /* sector value for first sector*/
	unsigned char   start_cyl;      /* track value for first sector */
	unsigned char   sysind;         /* system indicator 00=?? 01=DOS*/
	unsigned char   last_head;      /* head value for last sector   */
	unsigned char   last_sec;       /* sector value for last sector */
	unsigned char   last_cyl;       /* track value for last sector  */
	long    lowsec;                 /* logical first sector         */
	long    size;                   /* size of partion in sectors   */
};

/*===========================================================================*
/* variables */
/*===========================================================================*/

static unsigned char buffer[BLOCK];      /* general buffer for drive */
static unsigned char mbr[BLOCK];         /* master buffer for drive */
static int part;                         /* partition to be booted */
static int debug;                        /* debug flag */
static unsigned char vecbuf[VECLEN];     /* vector buffer */

/*===========================================================================*
 *                              disk_io                                      * 
 *===========================================================================*/
/* realizo funciones de I/O sobre disco */
int disk_io(int func, int disk, int track, int head, int sector, char *buf)    
{
   int byt, fnc,i;
   char *access;        /* pointer to function type */
   char *ptr;
   
   fnc = func;       /* copy function code */
   ptr = buf;        /* copy buffer pointer */

   switch (fnc) {
	 case GETSTATUS : access = "Status";  break;
	 case READVBLK  : access = "Read";    break;
	 case WRITEVBLK : access = "Write";   break;
	 case VERIFYTRK : access = "Verify";  break;
	 default :        access = "Unknown"; break;
	 }

   /* leo o escribo bloque del diskette */

   if (fnc == READVBLK) {
      if (debug)
	 printf("wini: cleaning buffer ...\n");

      for (i = 0; i < BLOCK; i++)
	 *ptr++ = '\0';
      }         

   if (debug)
      fprintf(stderr, "wini: %s access disk %d, head %d, track %d, sector %d\n", access,
	  disk, head, track, sector);

   byt = biosdisk(fnc, disk, head, track, sector, 1, buf);

   if (byt != 0)  /* trato de hacerlo 2 veces si hay error */
      byt = biosdisk(fnc, disk, head, track, sector, 1, buf);

   /* si hubo error imprimo mensaje en pantalla */

   if (byt != 0) {
      fprintf(stderr, "\nwini: %s error at disk %d (track %d, head %d, sector %d). ",(fnc == 2)? "Read": "Write",
			     disk, track, head, sector);
      prterr(byt);     /* print error */
      }
   return(byt);
}

/*===========================================================================*
 *                              prterr                                       * 
 *===========================================================================*/
prterr(int byt)       /* print biosdisk error */
{
      fprintf(stderr,"wini: ");

      if ((byt & 0x80) != 0)
	 fprintf(stderr, "Disk timed-out.\n");
      else if ((byt & 0x40) != 0)
	 fprintf(stderr, "Seek failure.\n");
      else if ((byt & 0x20) != 0)
	 fprintf(stderr, "Controller error.\n");
      else if ((byt & 0x10) != 0)
	 fprintf(stderr, "Data error on disk read (CRC).\n");
      else if ((byt & 0x08) != 0)
	 fprintf(stderr, "DMA overrun on operation.\n");
      else if ((byt & 0x04) != 0)
	 fprintf(stderr, "Request sector not found.\n");
      else if ((byt & 0x02) != 0)
	 fprintf(stderr, "Disk write protected.\n");
      else if ((byt & 0x01) != 0)
	 fprintf(stderr, "Ilegal command passed to driver.\n");
      else printf("Status = 0x%x \n", byt);

   return(0);
}

/*==========================================================================*
 *                              systype                                     *
 *==========================================================================*/
char *systype(type)
int type;
{
/* Convert system indicator into system name. */

  switch(type) {
	case 0:              return("  None ");
	case 1:              return("DOS-12 ");
	case 2:              return(" XENIX ");
	case 3:              return("XNX-OLD");
	case 4:              return("DOS-16 ");
	case 5:              return("DOS-EXT");
	case 6:              return("DOS-BIG");
	case 8:              return("  AIX  ");
	case 10:             return("  OPUS ");
	case 0x51:           return("NOVELL ");
	case 0x52:           return("  CPM  ");
	case 0x63:           return("386/IX ");
	case 0x64:           return("NOVELL ");
	case 0x75:           return(" PCIX  ");
	case 0x81:           return(" MINIX ");
	case 0x82:           return("LinuxSW");
	case 0x83:           return("LinuxEX");
	case 0xDB:           return("  CPM  ");
	case 0xFF:           return("BADBLKS");
	default:             return("Unknown");
  }
}

/*===========================================================================*
 *                              main                                         * 
 *===========================================================================*/
void main(int argc, char **argv)
{
    unsigned seg, off, segb, offb, segv, offv;
    char oem[10];
    int low_cyl, low_sec, low_head;
    struct part_entry *pe;
    FILE *fd;
    unsigned char dr; /* drive */
    char path[80];    /* path de trabajo */
    int i,j;          /* temp variable */
    
    part = 0;         /* invalid partition to be booted */
    path[0] = '\0';   /* init path string */
    dr = HD1;         /* primer hard disk */    

    printf("\nHard Disk Booter V%s - All rights reserved\n", VERSION);
    printf("1995 - C.M.T. Software Utilities Inc.\n\n");
    printf("\n1998 - Modified for HP200LX by Richard L. Dubs \n");

    j = 1;     /* points to first variable */
    if (argc > 1) {      /* must check each parameter */
       while (argv[j][0] == '-') {
	  switch (argv[j][1]) {             /* en funcion del flag         */
	     case 'd':               /* debug flag */
	     case 'D':
		       debug = 1;
                       printf("wini: debug flag on ...\n");
		       break;

	     case 'h':               /* hard drive flag */
	     case 'H':
                       j++;   /* points to new argument */
		       if (argv[j][0] == '1')
                          dr = HD1;
                       else if (argv[j][0] == '2')
                          dr = HD2;
                       else {
                          printf("wini: invalid drive %s.\n", argv[j]);
                          exit(1);
                          }
                       if (debug) {
                          printf("wini: hard drive %c selected.\n",(dr==HD1)?'C':'D');
                          }
		       break;

	     case 'w':               /* working path */
	     case 'W':
                       j++;   /* points to new argument */
                       strcpy(path,argv[j]);  /* copy new path */
                       if (debug) {
                          printf("wini: working path is \"%s\".\n", path);
                          }
                       strcat(path,"\\");  /* dir separator */    
                       break;

	     default:
		       printf("wini: invalid option \"%s\".\n", argv[j]);
                       exit(1);
		       break;
	     }
	  j++;   /* new pointer */
	  }
       if (argc == (j+1)) {
	  part = atoi(argv[j]); /* convert part number */
	  if ((part<=0)||(part>4)) {
	     fprintf(stderr,"wini: invalid partition %s.\n", argv[j]);
	     part=0;
	     }
	  }
       }

    printf("\nwini: Information for Hard Drive %c ...\n", (dr==HD1)?'C':'D'); 
    

    if (debug)
       printf("wini: reading master bootblock on hard disk ...\n");

    if (HDBOOT((unsigned int)dr)) {
       printf("wini: error reading master boot block from hard drive %d ...\n", (dr==HD1)?1:2);
       exit(1);
       }

    if (debug)
       printf("wini: checking master partition table info ...\n");

    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
       printf("wini: invalid master partition table.\n",i);
       exit(1);
       }

    dpl_partitions();    /* display partitions */
     
    if (part == 0)
       exit(0);

    if (debug)
       printf("\nwini: checking partition %d info ...\n", part);

    i = part;       /* points to partition */
    pe = (struct part_entry *)&mbr[TABLEOFFSET];
    while (--i > 0) pe++;  /* finds partition */
    
    if (pe->sysind == 0) {  /* avoid null partitions */
       printf("wini: null partition. Exiting.\n");
       exit(1);
       }

    low_cyl = (pe->start_cyl & 0xff) + ((pe->start_sec & 0xc0) << 2);
    low_sec = pe->start_sec & 0x3f;
    low_head = pe->start_head;

    if (RDBOOT((unsigned int)dr, low_cyl, low_head, low_sec)) {
       printf("wini: error reading boot block ...\n");
       exit(1);
       }

    if (debug)
       printf("\nwini: checking partition %d signature ...\n", part);

    if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
       printf("wini: invalid partition signature.\n",i);
       exit(1);
       }

    strncpy(oem,&buffer[3],8);
    oem[8] = '\0';

    printf("\nPartition %d - OEM : %s\n",part, oem);

    strcat(path, VECTOR_FILE);    /* encadeno ambos paths */
/*
    if (debug)
       printf("\nwini: loading vector area into core (%s) ...\n", path);

    if ((fd = fopen(path,"rb")) == NULL) {
       perror("fopen vector file");
       exit(errno);
       }

    if (fread(vecbuf,sizeof(char),VECLEN,fd) != VECLEN) {
       perror("fread vector file");
       exit(errno);
       }
*/
/*    (void)fclose(fd);  */     /* close vector file */

    if (debug) {
       seg = peek(0,(0x13*4)+2);
       off = peek(0,0x13*4);
       printf("\nwini: int 13h points to %x:%x .\n", seg,off);
       }

    segb = FP_SEG(buffer);    /* get segment from buffer */
    offb = FP_OFF(buffer);    /* get offset */

    segv = FP_SEG(vecbuf);    /* far pointer to vector buffer */
    offv = FP_OFF(vecbuf);    /* get offset */

    if (debug) {
       printf("\nwini: buffer address from %x:%x ...\n", segb, offb);
       printf("\nwini: vector address from %x:%x ...\n", segv, offv);
       }

    if (debug)
       printf("\nwini: jumping to boot code ...\n");

    /* copy boot code to 0:7c00 */
    asm push    ds
    asm push    es
    asm push    di
    asm push    si
    asm cld
    asm mov     ax, segb         /* segment to buffer */
    asm mov     ds, ax
    asm mov     si, offb         /* offset to buffer */
    asm mov     ax, 7c0h        /* segment to destination */
    asm mov     es, ax
    asm mov     di, 0
    asm mov     cx, BLOCK       /* size */
    asm rep movsb
    asm pop     si
    asm pop     di
    asm pop     es
    asm pop     ds

    /* copy code from vector buffer to vector area */
/*
    asm push    ds
    asm push    es
    asm push    di
    asm push    si
    asm cld
    asm mov     ax, segv    */    /* loads ds (source segment) */
/*    asm mov     ds, ax
      asm mov     si, offv  */    /* source offset */
/*    asm mov     ax, 0
      asm mov     es, ax    */    /* load es (destination segment) */
/*    asm mov     di, 0     */    /* destination offset */
/*    asm mov     cx, VECLEN  */    /* vector area length */
/*    asm rep movsb
    asm pop     si
    asm pop     di
    asm pop     es
    asm pop     ds
*/
    /* get pointers to partition area */
    
    seg = FP_SEG(pe);        /* far pointer to vector buffer */
    off = FP_OFF(pe);        /* get offset */

    asm mov     ax, seg      /* segment to es */
    asm mov     es, ax
    asm mov     si, off      /* offset to si */
    /* V1.1 modificacion se pasa variable dr en dl con disco de boot */
    asm mov     dl, dr       /* drive number */

    /* jmp 0:7c00h to code */
    asm db      0eah,00,07ch,0,0

}

/*===========================================================================*
/* dpl_partition : display partition info from structure
/*===========================================================================*/
dpl_partitions()
{
	struct  part_entry      *pe;
	int     i;

	printf("\nPartition      Type        SysId     Begin      End     Act  Offset     Size\n");
	pe = (struct part_entry *)&mbr[TABLEOFFSET];
	for (i = 1 ; i <= 4 ; i++) {
		dpl_entry(i,pe);
		pe++;
	}
}

/*===========================================================================*
/* dpl_entry : display an entry 
/*===========================================================================*/
dpl_entry(number,entry)
int     number;
struct  part_entry      *entry;
{
	int     low_cyl,high_cyl,temp,low_sec,high_sec,low_head,high_head;
	char    *typestring;
	char    active;
	char    begstr[80];
	char    highstr[80];

	if (entry->sysind == 0)
	   return;

	typestring = systype(entry->sysind);

	printf("%5d         %s       %3d ",number,typestring,entry->sysind);
	temp = entry->start_sec & 0xc0;
	low_cyl = (entry->start_cyl & 0xff) + (temp << 2);
	low_sec = entry->start_sec & 0x3f;
	low_head = entry->start_head;
	temp = entry->last_sec & 0xc0;
	high_cyl = (entry->last_cyl & 0xff) + (temp << 2);
	high_sec = entry->last_sec & 0x3f;
	high_head = entry->last_head;
	sprintf(begstr,"%d/%d/%d",low_cyl,low_sec,low_head);
	sprintf(highstr,"%d/%d/%d",high_cyl,high_sec,high_head);
	printf("%10s  %10s", begstr,highstr);
	if ((entry->bootind & 0xff) == 0x80)
		active = 'A';
	else
		active = 'N';
	printf("   %c %8ld %8ldKb\n",active, entry->lowsec, entry->size/2);
}

