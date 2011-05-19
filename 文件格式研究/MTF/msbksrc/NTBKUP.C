/* ntbkup.c
   Copyright (C) 2003 William T. Kranz
   Check http://www.fpns.net/willy/msbackup.htm for updates.
   contact info above or via snailmail: p.o. box 333, bradford, nh 03221

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-----------------------------------------------------------------
   History:

   12/29/03 rename xpbkup.c version 1.03 and change to
   the MTF structures which I was forcing with the _MTF define.
   12/31/03 seems to be working.  Should add CSUM stuff and
   do a little more testing, but think its there.
   01/01/04 added FOFFSET logic for 4GB
      also tweak do_file() changing return value if read file
      and try to add CSUM logic.  Think I have it, but not
      quite sure why my padding algorithm works!

   Interesting MSVC 5.0 cl seemed to promote the addition of
   two WORDs to sizeof(int).  see disp_unicode() calls.
   I moved addition of params outside paramater block
   ie ud = start+end
   disp_unicode(start,ud) instead of disp_unicode(start,start+end)
   and warnings went away.  Seems dumb!

   watch leading '/' stuff in do_filter() logic

   01/15/04 add prt_foff() to handle FOFFSET display
   gets a bit messy, I only parse command line args for start
   position as longs, then cast so limits some of my searches.

   changed some of returns link do_tfdd() to FOFFSET
   01/19/04 fix some Linux -Wall warnings about unused vars
   and add my own strupr() if unix (may be a problem in CYGWIN...)
   01/23/04 change version to 1.04, change -p to -l option
   to limit based on path and use -p in future as I do in msqic
   Add mode & LPATH define to control new -l option instead of depending 
   on path != null.  Want to use path in XTREE mode also

   01/24/04 make return from do_file and FOFFSET, should have been
            add SILENT to mode in main() for most options
   01/27/04 add set exclusion concept to tree creation, if -s
            is used, it limits what's added to directory tree
            -p no call do_redirect allowing * and + as terminator
   03/16/04 bump to version 1.03 see nttree and -p issue
            also tweak mode flags for LPATH so display less info
   05/17/04 bump to version 1.04 tweat is_match() for multiple '.'
            and nttree.c:find_path() to allow network drive specifications
   06/02/04 bump version to 1.05, add mode argument
            to do_dirb() so don't display directories if mode & SILENT
   07/08/04 bump version to 1.06, change do_file() so that it creates
            an empty file for FILE tags where there is no STAN stream
            remove obsolete error message regarding 'failed to find STAN'
   04/05/05 bump version to 1.07 copy prior source to ntbkup04.c
            correct some bugs parsing args, -s and -f
            add include get_foff.c for parsing 64 bit offsets

            may be getting carried away, but change type of len
            in fnd_target() to FOFFSET, oops it had an error also
            change how deal with rd

            add optional end option via -j#:# for main loop
    12/20/06 change version to 1.07b to reflect change in nttree increasing
             LN_SZ from 255 to 600
    12/30/06 change version to 1.07c as surround MAX_PATH define
             with an ifndef to allow changing on command line and
             increase default for 25 to 100.  Also add display at startup
-------
   per dos.h: File attribute constants 

#define _A_NORMAL	0x00	 Normal file - No read/write restrictions 
#define _A_RDONLY	0x01	 Read only file 
#define _A_HIDDEN	0x02	 Hidden file 
#define _A_SYSTEM	0x04	 System file 
#define _A_VOLID	0x08	 Volume ID file 
#define _A_SUBDIR	0x10	 Subdirectory 
#define _A_ARCH 	0x20	 Archive file 

*/

#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h> // for toupper()
#include <string.h> // define memset
#include <sys/types.h>
#include <sys/stat.h>   /* for open() defines */

#if  defined(MSDOS) || defined(_WIN32) 
# ifdef MSDOS
#  define OS_STR "MSDOS"
# else
#  define OS_STR "WIN32"
# endif
#define DELIM '\\'

#include <memory.h>
#include <io.h>         // for lseek
#include <sys/utime.h>
#else
#  ifdef __CYGWIN__
#    define OS_STR "CYGWIN" 
#else
#    define OS_STR "LINUX"
#endif
#include <unistd.h>     // for lseek, read, write
#include <utime.h>
#define DELIM '/'
#define strnicmp strncasecmp
#define stricmp  strcasecmp
#endif

#ifdef _WIN32
// this redefinition must be last!  ie after io.h
#define lseek _lseeki64
#endif

#include "ntbkup.h"  // includes FOFFSET defines

#include "get_foff.c"

// a prototypes for functions called before defined
FOFFSET do_file(int fp, FOFFSET soff,int mode); // uses global char *filter
// prototypes for functions in nttree.c
DIR_LIST *find_tree(DIR_LIST *next,char *str);
DIR_LIST *find_path(DIR_LIST *next, char *str);
void disp_tree(DIR_LIST *d);
int add_tree_node(DIR_LIST **root, FOFFSET off, int key);
int path_extract(int fp, DIR_LIST *node,unsigned long *fskip); // not used in this module
int get_paths(char *fn,struct path_elem paths[],int sz);
int do_redirect(int fp, DIR_LIST *root,struct path_elem *paths,int rcnt);




#define LBUF_SZ 512  // local buffer on stack

BYTE buf[XPBLK_SZ];  // used by most read routines. Assumes
                     //  all blocks smaller than this, checks if correct

char tree_path[PATH_LEN+1]; // make this global to simplify recursion stack

#ifdef _4GB
#define MAXINC 0x7fffffffL

/* FOFFSET is an unsigned long when _4GB is defined
   this limits SEEK_CUR and SEEK_END to calls with off == 0
   to get file size or current position, ie not a generic routine
*/
FOFFSET lseek4(int fp,FOFFSET off,int origin)
{
    FOFFSET roff,lret=LSEEK_ERR;
    long inc;
    if(origin == SEEK_CUR || origin == SEEK_END)  
    {
        if(off != 0L)
            printf(
"\nfatal error lseek called with none zero offset for SEEK_CUR or SEEK_END");
        else
            return((FOFFSET)lseek(fp,0L,origin));
    }
    // at two calls may be required
    else if(origin == SEEK_SET)
    {
        while(lret == LSEEK_ERR)
        {
           if(off <= MAXINC)
              inc = off;
           if((roff = lseek(fp,inc,origin)) == LSEEK_ERR)
              break;
           off -= inc;
           if(off == 0)
              lret = roff; // we are done
        } 
    }
    return(lret);
}
#endif



// see ntbkup.h for index defines into array below for is_keyword()
char *keywords[] = {
  "TAPE",  // 0
  "SFMB",
  "SSET",  // 2
  "VOLB",
  "DIRB",  // 4
  "FILE",  // 5
  "ESET",
  "EOTM",
  "CFIL",
  "ESPB", // 9
// MTF_MAX_BLK = ESPB = 9 rest are stream hdrs, although some on blk boundry
  "TSMP",
  "TFDD",  // 11  
  // from here on not on XPBLK_SZ boundry, above seem to be
  "NACL",  
  "NTQU",  // 13 
  "CSUM",
  "STAN",
  "SPAD",
  "FEND",  // 17 terminates TFDD region
  "NTOI",   // follows CSUM in an NACL region
  NULL
};

// return -1 on failure, or index into keywords[]
int is_keyword(char *str)
{
    int i=0,j;
    if(*((long *)str) == 0)
       return(NUL_KEY);
    while(keywords[i] != NULL)
    {
         for(j=0;j<4;j++)
              if(*(keywords[i]+j) != *(str+j))
                   break;
         if(j == 4)
              return(i);
         i++;
    }
    return(-1);   
}

void get_xpdatetime(BYTE t[],unsigned short *yr, BYTE *mon, BYTE *day, BYTE *hr, 
               BYTE *min, BYTE *sec)
{
    *sec = t[4] & 0x3f; // 6 bits
    *min = ((t[3] << 2) + (t[4] >> 6)) & 0x3f; // next 6 bits
    *hr = t[3] >> 4; // low order 4 bits of hour.
    if(t[2] & 1)  // high bit of hour  for full 5 bit value
        *hr += 0x10;
    *day = (t[2] >> 1) & 0x1f; // 5 bits
    *mon = ((t[1] << 2) + (t[2] >> 6)) & 0xf;  // 4 bits
    *yr = t[0];
    *yr = (*yr << 6) + (t[1] >> 2);  // 14 bits
}


// warning no length qualifiers allowed in fmt, "ld" or "lx" expected
void disp_offset(char *fmt,FOFFSET off)
{
    unsigned long ll=0x8000000L;
    if(off < 0)
       printf("< 0");
    else if(off < ll) // can display directly
    {
       ll = off;
       printf(fmt,ll);
    }
    else // its a 64 bit long long, break it up
    {
       ll = off >> 32;
       printf(fmt,ll); // high order bytes
       ll = off & 0xffffffff;
       printf(fmt,ll); // low order bytes
    }
}

void disp_xpdatetime(BYTE t[])
{   
    WORD yr;
    BYTE mon,day,hr,min,sec;
    char str[4];
    get_xpdatetime(t,&yr,&mon,&day,&hr,&min,&sec);
    if(hr < 12)
    {
       strcpy(str,"AM");
       if (hr == 0)
           hr = 12;
    }
    else
    {
       strcpy(str,"PM");
       if(hr > 12)
          hr -= 12;
    }
    printf("%02d/%02d/%04d  %02d:%02d:%02d %s",
              mon,day,yr,hr,min,sec,str);

}

/* both unicode routines assume global buf[start] is start of unicode
   and MAX is next char after end of string
   ignores every 2nd char, ie treats as ascii
   return # chars transfered just for giggles
*/
int disp_unicode(WORD start, WORD max)
{
    int len = 0;
    char *ch= (char*)buf+start;
    while(start < max)
    {
         fputc(*ch,stdout);
         ch+=2;
         start+=2;
         len++;
    }
    return(len);
}
/* my original notes suggested if it was just the root the unicode
   started with '/' but if there was a path it started with first
   char. 

   Above seem incorrect.  There is never a leading '/'
   The path delimiter is a WORD = 0 in the string.
   These should be replaced with the OS specific DELIM
   Prior to 1/23/04 I used '/'
   I insert a leading '/' if the path does not start with 0
      AND contains more than one char.
   ie its not just the root, but sub dir spec follows.
   replace all occurances of 0 with DELIM in string which
   always ends with a 0.
*/
int disp_unipath(WORD start, WORD max)
{
    int len = 0;
    char *ch= (char*)buf+start,cout;
    // odd, if root start with '/', else start with 1st path and no '/'
    // maybe it has a zero for last '/' ??
    if(max - start > 0 && *ch != 0)
        fputc(DELIM,stdout);
    // else code below puts up the leading and trailing '/' is 0
    while(start < max)
    {
         if((cout = *ch) == 0)
            cout = DELIM;

         fputc(cout,stdout);
         ch+=2;
         start+=2;
         len++;
    }
    return(len);
}

/* Warning returns chars read, but may insert a leading '/'
   so string is longer by 1 char
*/
int getn_unipath(char *ch, WORD start, WORD max, int len)
{
    char inc=0;
    if(max - start > 0 && *((char*)buf+start) != 0)
    {
        *(ch++) = DELIM;
         inc = 1;
    }
    if(start+2*(len-inc) < max)
       max = start+2*(len-inc);
    len = 0; // use as counter now for chars read, does not include 1st DELIM
    // else code below puts up the leading and trailing '/' is 0
    while(start < max)
    {
         *ch = *((char *)buf+start);
         if(*ch == 0)
            *ch = DELIM;
         ch++;
         start+=2;
         len++;
    }
    *ch = 0; // always terminate
    return(len);
}


// copy uni-code to ascii buffer with max len chars
int getn_unicode(char *ch,WORD start,WORD max,int len)
{
    if(start+2*len < max)
       max = start+2*len;
    len = 0; // use as counter now
    while(start < max)
    {
         *(ch++) = buf[start];
         start+=2;
         len++;
    }
    *ch = 0; // assume space after len for NUL, always terminate
    return(len);
}


// do_ routines below which don't return a long assume all data in one block
//     those that do return a long return # bytes in region or -1 if don't know
void do_tape() // assume fits in one block
{
    MTF_TAPE *tptr = (MTF_TAPE *)buf;
    MTF_TAPE_ADR *sdat;
    WORD end;
    fputc('\n',stdout);
    sdat = &tptr->media_name;
    end = sdat->offset+sdat->size;
    // add end for MSVC 5.0, gives warning if don't construct per below
    disp_unicode(sdat->offset,end);
    fputc('\n',stdout);
    sdat = &tptr->media_label;
    end = sdat->offset+sdat->size;
    disp_unicode(sdat->offset,end);
    fputc('\n',stdout);
    sdat = &tptr->software_name;
    end = sdat->offset+sdat->size;
    disp_unicode(sdat->offset,end);
    fputc('\n',stdout);
    fputc('\n',stdout);
}

void do_sset(int set) // assume fits in one block
{
    WORD end;
    MTF_SSET *sptr = (MTF_SSET *)buf;
    printf("\nSet %d: ",sptr->set_num);    
    printf("\nName: ");
    end = sptr->set_name.offset +sptr->set_name.size;
    disp_unicode(sptr->set_name.offset,end);
    printf("\nDescription: ");
    end = sptr->set_descript.offset +sptr->set_descript.size;
    disp_unicode(sptr->set_descript.offset,end);
    printf("\nUser: ");
    end = sptr->user_name.offset +sptr->user_name.size;
    disp_unicode(sptr->user_name.offset,end);
    printf("\n");
}

// note main reads XPBLK_SZ bytes, first MTF_STREAM_HDR will always fit
// data following may...
// add display arg 12/8/05 if 0 advances without display to skip region
FOFFSET do_tfdd(int fp, FOFFSET diroff,char display)
{
    int i,j,key,rd;
    WORD ud,*wptr,off; 
    FOFFSET foff=0; // foff for cumulative read offset
    // initializations below only valid after reads
    MTF_FDD_FILE *file = (MTF_FDD_FILE *)buf;  // same as DIRB
    MTF_FDD_VOLB *volb = (MTF_FDD_VOLB *)buf;
    MTF_FDD_HDR *head = (MTF_FDD_HDR *)buf;
    
    off = sizeof(MTF_STREAM_HDR); // skip FDD stream header
    /* the sub sections are all part of FDD, contained in .var_sz
       should terminate with FEND, but watch for next block ID  
       next MTF_FDD_HDR starts with {WORD length, BYTE tag[]} ie 8 bytes
    */
    while((key=is_keyword(buf+off+sizeof(WORD))) != FEND && key > -1)
    {
         if(key != VOLB && key != DIRB && key != FILE_K)
             break; // invalid key for FDD area
         wptr = (WORD *)(buf+off); // length next region
         foff+=off;
         off = *wptr; // next offset after read
         i = j = 0;
         rd = *wptr + 8; // makes next length and tag valid
         if(rd > XPBLK_SZ ||(FOFFSET) lseek(fp,diroff+foff,SEEK_SET) != diroff+foff ||
            (j = read(fp,buf,rd)) != rd)
         {
            printf("\nTFDD fatal read error");
            return(LSEEK_ERR);
         }
         else if(display)
         {
            printf("\n%4.4s: ",head->tag);
            if(key == VOLB)
            {
                if(volb->dev_name.size > 0)
                {
                   printf("\nDevice Name: ");
                   ud = volb->dev_name.offset+volb->dev_name.size;
                   disp_unicode(volb->dev_name.offset,ud);
                }
                if(volb->vol_name.size > 0)
                {
                   printf("\nVolume Name: ");
                   ud = volb->vol_name.offset+volb->vol_name.size;
                   disp_unicode(volb->vol_name.offset,ud);
                }
                if(volb->mach_name.size > 0)
                {
                   printf("\nMachine Name: ");
                   ud = volb->mach_name.offset+volb->mach_name.size;
                   disp_unicode(volb->mach_name.offset,ud);
                }

            }
            else if (key == DIRB)
            {
                if(file->name.size)
                {
                   ud = file->name.offset+file->name.size;
                   disp_unipath(file->name.offset,ud);
                }
            }
            else if(key == FILE_K)
            {   
                printf(" %8lu @ blk = %4lu  ",
                     (DWORD)head->disp_sz,(DWORD)head->fmt_adr);
                disp_xpdatetime(file->modify); 
                printf("   ");
                // this is the long name [need to add MTF_TAPE_ADR]
                ud = file->name.offset+file->name.size;
                disp_unicode(file->name.offset,ud);
            }
         }
         if(key == FEND)
         {
            if(display)
               printf("  end of directory\n");
            foff+= 8; // we read last 8 bytes, add it
            break;
         }

    }
    return(foff); // bytes read

}



/* 
   was unsure of this.  MTF clearifies
   12/08/03 with additional examples, below isn't generic
   Phillip's C: hits an NACL region first and its different.
   I think its safer to just scan for next tag than guess at this
   leave this routine for reference, but don't call it!
   This worked when keyword at offset tag_head.var_sz was NTQU

   Below was developed from only two drives so may well be
   specific to them.  No idea how many other keywords might be here!

    if((key = is_keyword(buf+off)) == NTQU)
    {
         wptr = (WORD *)(buf+off+20); // offset into NTQU region
         lret = off + *wptr;  // data length of DIRB, may have word checksum at end?
    }    above takes one to within 3 bytes of end of SPAD region
         below is better.

    Algorithm below will fail if doesn't find an SPAD and 
    on return(-1) should resync
*/
long do_dirb(int fp,FOFFSET soff,int mode, int *nmlen)
{
    long lret=0;
    MTF_DIRB *dir = (MTF_DIRB *)buf;
    WORD sz,start;
    int i;
    char ch,*cptr;

    if(dir->head.str_type) // have OS string
    {
        sz = dir->head.OS_data.size;
        start = dir->head.OS_data.offset;
#ifdef TEST
        printf("\nOS data[%d] @ 0x%x ",sz,start);
#endif
    }
    sz = dir->name.size;
    start = dir->name.offset;

    if(!(mode & SILENT) && sz && start+sz <= XPBLK_SZ)
    {
        printf("\nDir Name[%d]: ",sz); // leading slash off root assumed
        disp_unipath(start,(WORD)(start+sz-1));
    }

    // return actual name length, copy up to max length to global tree_path[]
    i = getn_unipath(tree_path,start,start+sz-1,PATH_LEN);
    if(i != sz/2) // read truncated
       i = -1;
    else
       i = 1;
    *nmlen = i * strlen(tree_path); // actual string length always valid
                  
    return(lret);
}

int match_path(char *path,int path_len)
{
   int i,sz,suc = 0;
   char *ch;
   MTF_DIRB *dir = (MTF_DIRB *)buf;
   sz = dir->name.size;
   ch = buf+dir->name.offset;
   if(sz/2 >= path_len && dir->name.offset+path_len <= XPBLK_SZ)
   {
       if(*(path+path_len-1) != '*' && sz/2 != path_len)
          path_len = -1; // no wild card, no match
       for(i=0;i<path_len;i++)
       {
          if(*(path+i) == '*')
          {
              suc++; // wild card matches all rest
              break;
          }
          if(*ch != *(path+i))
              break;
          ch += 2;
       }
       if(i == path_len)
          suc++; // it works


   }
   return(suc);
}

#ifdef NOTNEEDED //was unix
// microsoft libraries include this
char * strupr(char *ch)
{ char *ret = ch;
  while(*ch != 0)
  {
    *ch = toupper(*(ch));
    ch++;  // don't do this inside macro above or occurs twice!
  }
  return(ret);
}
#endif


int is_match(char *str,char *filter)
{
    char ret=1,dotcnt=0,*tch=str;
#ifdef MSDOS  // use full case sensitive path in WIN32 and unix
    strupr(str); // DOS name compare
    strupr(filter);
#endif
    while(*tch != 0)
         if(*tch++ == '.') dotcnt++;
    while(*str != 0 && ret == 1)
    {
        if(*str == '.')
        {
            dotcnt--;
            if(dotcnt == 0 && *filter == '*')
              filter++;  // advance to next portion of filter
        }
        if(*filter == '*' || *filter == '?') // match all
            str++;
        else if (*filter++ != *str++)
            ret = 0;
        if(*filter == '?')
            filter++;        

    }
    return(ret);    
}

/* change logic to parse all, both blocks and streams.
   assume start with buf positioned on a generic header
   The start position of this buffer is passed in
   ah ha, seems to terminate with the next blocks key
   ie key <= MTF_MAX_BLK
   The last head.var_sz takes one to EOF or the next block ID

   oops can't init like I did with off = XPBLK_SZ as
   then my displayed offsets off by 1 block.
   Add blk counter and set to -1 to force init.
   the ESET takes one to TFDD via head.var_sz = 0x400
   which just happens to be beyond the current block!
   add verb flag, do display if true
*/
int do_streams(int fp,FOFFSET coff, FOFFSET end)
{
    int key,rd,suc=0;
    long blk = -1;
    MTF_DB_HDR *hdr; 
    FOFFSET off=0,toff;
    MTF_STREAM_HDR  *shdr;
    off = 0; // set to force reposition on 1st pass
    while(1)
    {
        // MTF_STREAM_HDR is larger than my struct tag_head
        // use blk to force initial read on 1st pass
        if(blk == -1 || off + sizeof(MTF_STREAM_HDR) > XPBLK_SZ)
        {
            coff += off;
            off = 0;
            if ((FOFFSET)lseek(fp,coff,SEEK_SET) != coff ||
               (rd=read(fp,buf,XPBLK_SZ)) != XPBLK_SZ)
            {
               if((off=(FOFFSET)lseek(fp,0L,SEEK_END)) != coff+rd)
               {
                   printf("\nEOF at ");disp_foffset(off);
               }
               else
               {
                   printf("\ndo_streams() file advance error at ");
                   disp_foffset(coff);
               }
               break;
            }
            blk = coff/XPBLK_SZ; // current block # were are in
            // may have started read anywhere in this block
            if(LSEEK_ERR != end)
               if(coff >= end)
                   break; // end of range
        }
        key=is_keyword(buf+off);
        toff = coff+off;
        if(key == -1)
        {
            printf("\nkey 0x%lx  not recognized at",*((DWORD*)(buf+off)));
            disp_foffset(toff);
            printf(", skip to next block boundry");
            off = XPBLK_SZ;
        }
        else if(key >= 0 && key <= MTF_MAX_BLK)
        {
            hdr =(MTF_DB_HDR *)(buf+off);
            printf("\n\n%4.4s Block Region attrib 0x%08lx @ ",hdr->tag,hdr->attrib);
            disp_foffset(toff); 
            printf(" contains:");
            off += hdr->var_sz;
        }                     
        else if(key > MTF_MAX_BLK)
        {
            shdr = (MTF_STREAM_HDR *)(buf+off);
            printf("\n   %4.4s  attrib 0x%04x 0x%04x @ ",
                     shdr->tag,shdr->sys_attrib,shdr->media_attrib);
            disp_foffset(toff); 
            off += sizeof(MTF_STREAM_HDR)+shdr->length; // skip struct + data
        }
        else
            break;
        // at least on case with ESET were (hdr->var_sz % 4) != 0
        // most shdr->length need modulo work
        if((rd = off % 4) != 0)
            off += 4-rd; // next DWORD boundry 
    }

    return(suc);
}

unsigned char mondays[] = {31,28,31,30,31,30,31,31,31,30,31,31};
#define BASEYR 1970
DWORD unix_time(BYTE t[])
{
    DWORD ret = t[4] & 0x3f; // low order 6 bits for seconds;
    int i,tmp;
    long mult = 60;
    tmp = ((t[3] << 2) + (t[4] >> 6)) & 0x3f; // next 6 bits for min
    ret += mult * tmp;
    mult *= 60;
    tmp = t[3] >> 4; // low order 4 bits of hour.
    if(t[2] & 1)  // high bit of hour  for full 5 bit value
        tmp += 0x10;
    ret += mult * tmp;
    mult *= 24;
    tmp = (t[2] >> 1) & 0x1f; // 5 bits for day of month
    if(tmp > 0)
       tmp--;
    ret += mult * tmp;
    tmp = ((t[1] << 2) + (t[2] >> 6)) & 0xf;  // 4 bits for month of year
    if(tmp > 12) 
        tmp = 12; // an error, don't let array go out of bounds
    if(tmp > 0)
        tmp--; // make it a zero based array index
    for(i=0;i<tmp; i++)
       ret += mult * mondays[i]; // add in prior months in this year

    tmp = t[0];
    tmp = (tmp << 6) + (t[1] >> 2);  // 14 bits for year this is base 0
    if(tmp < BASEYR)
       tmp = BASEYR;
    i = BASEYR;
    while(i < tmp) // step through years from base checking for leap years
    {
       if((i %100) == 0 || (i % 4) == 0)
           ret += mult * 366; // leap year
       else
          ret += mult *365;
       i++;
    };
    return(ret);
}

// may want to add a VERBOSE mode to query about extract later
// note this is called both to skip over data, and extract depending on mode
// CAREFUL my lbuf use a little screwy with the pad factor
// may read further into it than LBUF_SZ bytes?
BYTE lbuf[LBUF_SZ+4]; // make global so not on stack

// global filter, set in main if used
char *filter = NULL;

FOFFSET do_file(int fp, FOFFSET soff,int mode)
{
    int fo=EOF,rd,ir,iw,key;
    WORD wlen;
    BYTE docksum=0,pad=0; // local buffer
    DWORD flen=0,tlen,cksum=0,*cptr; // where we are now
    FOFFSET off,toff;
    MTF_FILE  *ff; // fixed region
    NT_FILE *ff2; // 2nd fixed region
    MTF_STREAM_HDR *shdr;
    struct utimbuf times;
    ff = (MTF_FILE *)buf; // point to start local buffer
    /* ff->var_sz points to first in list of streams as before
       but now know structure, and should be able to skip ahead
       to find the file data in STAN
       1st cut lets assume its in the buffer
    */
    off = ff->head.var_sz;
    key = -1;
    while(off < XPBLK_SZ && key != STAN)
    {
        shdr = (MTF_STREAM_HDR *)(buf+off);
        key=is_keyword(buf+off);
        if(key == NUL_KEY || (key >=0 && key <= MTF_MAX_BLK))
           break; // end of this data region
        off += sizeof(MTF_STREAM_HDR); // always add header length
        if(key != STAN) // advance to next stream
        {
           off += shdr->length; // skip data
           if((rd = off % 4) != 0)
              off += 4-rd; // next DWORD boundry
        }
        else // the file is the data
           flen = shdr->length; // warning its a QWORD in stream header!
    }

    /*   Notes:
    as of 6/8/04 try removing error message below, let flen==0 for an empty file
    If there is no file data, the file is empty but should be created

    if(key != STAN)
    {
         mode &= ~EXTRACT; // cancel xtract if active
         mode |= SILENT; // display nothing more
         printf("\ndo_file() failed to find STAN");
    }
    
     below was a much older note when first found MTF spec:
     this is a simple patch to my older none MTF code
         it stops searching the stream on detecting STAN
         There is more after this, see do_stream() with -v option
         Typically at least a checksum
    */

    if(flen > 0 && (mode & EXTRACT || !(mode & SILENT)) )
    {
        printf(" data from "); 
        toff = soff+off;
        disp_foffset(toff);
        printf(" to ");
        toff = soff+off+flen;
        disp_foffset(toff);
     }


    // if mode | extract, get the file
    if(mode & EXTRACT)
    {
        if(shdr->media_attrib & STREAM_CHECKSUMED)
            docksum++; // only updated if actually do extract, otherwise ignore

        wlen = ff->name.offset; // LFN filename length
        // 1st string from end struct to var_sz
        ff2 = (NT_FILE *)(buf+ff->head.OS_data.offset); // 2nd fixed region
        // ff2->nmlen > 0 if 2nd MSDOS file name exists
#ifndef HAS_INT64   // it doesn't handle long file names either!
        if(ff2->size > 0) // get the msdos name if it exists
        {
           wlen = ff->head.OS_data.offset+ff2->offset; // point to start short name
           getn_unicode(lbuf,wlen,(WORD)(wlen+ff2->size),15);
        }
        else
#endif     // always use LFN if HAS_INT64 =>(LINUX || WIN32)
           // use LFN if there is no MSDOS name, ie name is MSDOS 8.3 compatible
           getn_unicode(lbuf,wlen,(WORD)(wlen+ff->name.size),LBUF_SZ);

        // buf is never overwritten as use lbuf for copy
        // so ff->name etc stay valid
        // 12/4/03 add filter to skip some files
        if (filter != NULL && !is_match(lbuf,filter)) 
             docksum=0; //  no-op do nothing
        else if((fo=open(lbuf,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,
                        S_IREAD|S_IWRITE)) == EOF)
             printf("\nfailed to open %s for output",lbuf);
       
        else if(flen > 0 && (FOFFSET)lseek(fp,soff+off,SEEK_SET) != soff+off)
             // file data starts immediately after MTF_STREAM_HDR
             printf("\nfailed to seek to start of data");
        else
        {    // ff2->attrib is a DWORD, MSDOS byte is low order one, rest 0?
             printf("\nlength %6ld  atrib 0x%2x  ",flen,(BYTE)ff2->attrib);
             disp_xpdatetime(ff->modify);
             printf("\nextracing: %s: ",lbuf);
             // use local buffer for copy, preserve FILE info in buf[]
             tlen = flen;
             if(flen > 0 && docksum) 
                 docksum++; // increment to 2, we are trying extract
             // if flen == 0 skip check sum logic
             while(tlen > 0)
             {
                if(tlen > LBUF_SZ)
                   rd = LBUF_SZ;
                else
                {
                   rd = tlen; // tail end of the file
                   /* 1/1/04 add pading based on off+flen, ie enough
                      to get me to the next stream header, normally a CSUM
                   */
                   pad = (off+flen) % 4;
                   if(pad) // wasn't modulo of 4 bytes pad the read
                      pad = 4 - pad;
                   memset(lbuf,0,LBUF_SZ); // clear buffer before last read
                   // this is an attempt to get a valid cksum
                }
                if((ir=read(fp,lbuf,rd+pad)) != rd+pad ||
                   (iw = write(fo,lbuf,rd)) != rd) // only write rd bytes
                {
                   printf("\nio error writing file");
                   break;
                }
                else if(mode & VERBOSE)
                   fputc('.',stdout);
                tlen -= rd; // tlen goes to zero breaking us out of loop

                if(docksum) // do checksum logic
                {  /* below seems to work, but worries me
                      the MTF_STREAM_HDR is 22 bytes long
                      a two byte file would have a pad of 0, ir = 2
                      the cksum below would wrap two extra letters into lbuf
                   */
                   cptr = (DWORD*)lbuf;
                   for(iw=0;iw<rd;iw += 4)
                   {
                      cksum ^= *(cptr++);                      
                   }
                }
             }             
             close(fo);
             // recover file name into lbuf so can set timestamp
#ifndef HAS_INT64   // it doesn't handle long file names either!
             if(ff2->size > 0) // get the msdos name if it exists
             {
                 wlen = ff->head.OS_data.offset+ff2->offset; // point to start short name
                 getn_unicode(lbuf,wlen,(WORD)(wlen+ff2->size),15);
             }
             else
#endif       // always use LFN if HAS_INT64 =>(LINUX || WIN32)
             // use LFN if there is no MSDOS name, ie name is MSDOS 8.3 compatible
             getn_unicode(lbuf,wlen,(WORD)(wlen+ff->name.size),LBUF_SZ);

             // attempt to set time stamp and attribute with generic functions
             times.actime = times.modtime = unix_time(ff->modify);
             rd = 0;
             if(utime(lbuf,&times) != 0) // set time stamp
                rd = 1;

             ir = S_IFREG | S_IREAD; 
             if((ff->attrib & 1) == 0) // its writable
                 ir |= S_IWRITE;
             if(chmod(lbuf,ir) != 0)
                 rd |= 2;
             if(rd)
                printf("\nerror setting file: ");
             if(rd & 1)
               printf("timestamp ");
             if(rd & 2)
               printf("attributes"); 
             fputc('\n',stdout); // be sure there is an lf
        }
    }
    // always need to skip over file data, if extracted or not 
    off += flen+pad; // offset to end of data + pad alignment if extract 
    // WARNING for MTF should scan to end of stream list, and possibly do CSUM
    if(docksum > 1) // we tried to extract file, advanced fp, cksum is valid
    {
       shdr = (MTF_STREAM_HDR *)lbuf;
       if((rd = read(fp,lbuf,sizeof(MTF_STREAM_HDR))) != 
          sizeof(MTF_STREAM_HDR) || is_keyword(lbuf) != CSUM ||
          (ir = read(fp,lbuf+rd,(int)shdr->length)) != shdr->length)
           printf("\ninput error reading file checksum");
       else if(cksum != *((DWORD*)(lbuf+rd)) )
           printf(" - checksum error!");
       off += rd+ir;
    }
    return(off); // bytes to read or skip to get past file data
                 // Warning, still must advance to next block boundry
}

// warning, the input variables start and len are DWORD
// I was orignally lazy parsing in main, but change to
// len and start as FOFFSET 4/6/05
void fnd_target(int fp,DWORD targ,FOFFSET start,FOFFSET len)
{
    int i,rd;

    printf("\nsearching file for 0x%lx => ascii:%4.4s from offset ",
        targ,(char *)&targ);
    disp_foffset(start);
    printf(" to ");
    if(len == LSEEK_ERR)
        printf("EOF");
    else 
    {
        disp_foffset(start+len);
    }
    if((FOFFSET)lseek(fp,start,SEEK_SET) != start)
    {
       printf("\nseek to start failed");
       return;
    }

    while((len == LSEEK_ERR || len > 0) && (rd = read(fp,buf,XPBLK_SZ)) > 4)
    {        
        if(len > 0)
        {
           if(len < rd)
           {
              rd = len;
              len = 0;
           }
           else 
              len -= rd;
        }
        for(i=0;i<rd-4;i++)
           if(targ == *((DWORD *)(buf+i)) )
           {
              printf("\ntarget at offset ");
              disp_foffset(start+i);
           }
        start += rd;
    }
    printf("\nsearch ended at offset ");
    disp_foffset(start);
}

// for standard MTF header, {0x19 words + cksum}
int hd_cksum(WORD *wptr,int len)
{  
    int i;
    WORD cksum=0;
    for(i=0;i<len;i++)
         cksum ^= *(wptr++);  // XOR sum of word in header excluding cksum
    i = (cksum - *wptr);
    return(i); // zero means it was valid
}

// simplistic MTF dependant routine, assumes all strings fit in 1st block
int do_volb()
{
    WORD start,max,sz;
    int nmlen,i;
    MTF_VOLB *volb = (MTF_VOLB *)buf;
    start = volb->dev_name.offset;
    sz = volb->dev_name.size;
    max = start+sz-1;
    if(sz > 0 && max < XPBLK_SZ)
    {
       printf("\ndevice name: ");
       disp_unicode(start,max);
    }
    start = volb->vol_name.offset;
    sz = volb->vol_name.size;
    max = start+sz-1;
    if(sz > 0 && max < XPBLK_SZ)
    {
       printf("\nvolume name: ");
       disp_unicode(start,max);
    }
    start = volb->dev_name.offset;
    sz = volb->dev_name.size;
    nmlen = getn_unicode(tree_path,start,start+sz-1,PATH_LEN);
    return(nmlen);
}

#ifndef MAX_PATH
#define MAX_PATH 100
#endif

int main(int argc, char *argv[])
{
    struct path_elem paths[MAX_PATH];
    MTF_DB_HDR *th;
    DIR_LIST *root=NULL;
    int fp,i,rd,key,mode = 0,tmode,path_len,dir_len,rcnt=0,
        tset=0,cset=0; // target and current set values, tset = 0 for all
        // NTbackup uses 1 based sets, ie 1st is #1, 2nd is #2
    FOFFSET off = 0,skip,len,toff,start=LSEEK_ERR,end= LSEEK_ERR;
    DWORD targ=0;
    char *ch,flag,*fmt,*path=NULL;
    printf("\nNTBKUP Ver 1.07c compiled for %s with MAX_PATH = %d",
            OS_STR,MAX_PATH);
#ifdef _4GB
    printf("\n   max file size 4Gb");
#endif
#ifdef HAS_INT64
    printf("\n   compiled for 64 bit file offsets");
    if(sizeof(FOFFSET) != 2 * sizeof(long))
    {  
        printf("\nfatal error, however sizeof(FOFFSET) = %d",sizeof(FOFFSET));
        printf(
"\nif compiled with gcc, you probably forgot -D_FILE_OFFSET_BITS=64");
        exit(0);
    }
#endif
    printf("\nCopyright (C) 2003 William T. Kranz");
    printf("\nNTBKUP comes with ABSOLUTELY NO WARRANTY");
    printf(
"\nFree software distributed under the terms of the GNU General Public license");
    printf(
"\nSee http://www.gnu.org/licenses/gpl.html for license information");

    printf(
"\nCheck http://www.fpns.net/willy/msbackup.htm for Updates & Documentation\n");

    // debug aid, check structure sizes
    if(argc > 1 && strnicmp(argv[1],"-ss",3) == 0)
    {
         printf("\nknown strutures:");
         printf("\nsizeof(FOFFSET) = %d",sizeof(FOFFSET));
         printf("\nsizeof(MTF_TAPE_ADR) = %d",sizeof(MTF_TAPE_ADR));
         printf("\nsizeof(MTF_DB_HDR) = %d",sizeof(MTF_DB_HDR));
         printf("\nsizeof(MTF_STREAM_HDR) = %d",sizeof(MTF_STREAM_HDR));
         printf("\nsizeof(MTF_TAPE) = %d",sizeof(MTF_TAPE));
         printf("\nsizeof(MTF_VOLB) = %d",sizeof(MTF_VOLB));
         printf("\nsizeof(MTF_DIRB) = %d",sizeof(MTF_DIRB));
         printf("\nsizeof(MTF_SSET) = %d",sizeof(MTF_SSET));
         printf("\nsizeof(MTF_FILE) = %d",sizeof(MTF_FILE));
         printf("\nsizeof(MTF_FDD_HDR) = %d",sizeof(MTF_FDD_HDR));
         printf("\nsizeof(MTF_FDD_VOLB) = %d",sizeof(MTF_FDD_VOLB));
         printf("\nsizeof(MTF_FDD_DIRB) = %d",sizeof(MTF_FDD_DIRB));
         printf("\nsizeof(NT_FILE) = %d",sizeof(NT_FILE));

         fputc('\n',stdout); //cleanup for linux
         exit(0); // we are done
    }

    if(argc < 2)
    {
       printf(
"\nntbkup <file> [-x] [-l] [-p] [@<cmd>] [-c] [-d] [-f] [-j#] [-s#] [-t] [-v]");
       printf("\n    -x[filter] to unconditionally extract all files matching filter");
       printf("\n    -l<path> where full case sensitive path limits extract"); 
       printf("\n    -p<path> recursive path based directory extract"); 
       printf("\n    @<cmd> use path based extract and redirection command file");
       printf("\n       all extracts use [filter] from -x, default filter is *.*");
       printf("\n    -c to display catalog regions for TAG == TFDD");
       printf("\n    -d display directory tree from raw data region");
       printf(
"\n    -f<tag>[:start[:len]] finds 4 char tag, optional start pos and length");
       printf(
"\n    -j#[:#] jump to start position in file for data recovery (modulo 0x400)");
       printf("\n        optionally follow start offset with :# for an end offset");
       printf("\n    -s# to limit options above to a particular SET by #");
       printf("\n    -t[:start[:end]] display tags only from start to end");
       printf("\n    -v to set verbose mode");
       // -ss to show structure sizes is hidden
       fputc('\n',stdout); //cleanup for linux
       exit(0);
    }
    else // start at i = 1 as special case so can have -ss as 1st argument
       for(i=1;i<argc;i++)
       {
         if(strnicmp(argv[i],"-c",2) == 0)
              mode |= CATALOG|SILENT;        
         if(strnicmp(argv[i],"-d",2) == 0)
              mode |= TREE|DTREE|SILENT;        
         else if(strnicmp(argv[i],"-s",2) == 0 && 
                 sscanf(argv[i]+2,"%d",&rd) == 1 && rd > 0)
         {
              printf("\nresrict operations to backup set %d",rd);
              tset = rd;    
         }
         else if(strnicmp(argv[i],"-v",2) == 0)
         {
              mode &= ~SILENT;
              mode |= VERBOSE;
         }
         else if(strnicmp(argv[i],"-f",2) == 0)
         {
              ch = argv[i] + 2; // start target string
              if(strlen(ch) >= 4)
              {
                 targ = *((long *)ch);
                 start = 0L;len = LSEEK_ERR; // to eof
                 if((ch = strchr(ch,':')) != NULL) // have start
                 {
                    get_foffset(ch+1,&start); // aborts on error
                    if((ch = strchr(ch+1,':')) != NULL) // have len
                       get_foffset(ch+1,&len);
                 }
              }
              else
                   printf("\ninvalid argument for -f, min 4 chars required\n");
         }
         else if(strnicmp(argv[i],"-t",2) == 0)
         {
              start = 0L;len = LSEEK_ERR; // to eof
              get_foffset(argv[i]+2,&start); // aborts on error!
              if((ch = strchr(argv[i]+3,':')) != NULL) // there is a len
                 get_foffset(ch+1,&len);
              mode |= TAGS;
         }
         else if(strnicmp(argv[i],"-j",2) == 0)
         {
              get_foffset(argv[i]+2,&toff); // aborts on error!

              if((rd = (toff % XPBLK_SZ)) != 0)
                  toff -=rd; // modulo 0x400
              off = toff; 
              if((ch = strchr(argv[i]+3,':')) != NULL) // there is an end
                 get_foffset(ch+1,&end);
        }
         else if(strnicmp(argv[i],"-p",2) == 0)
         {
              mode &= ~LPATH; // clear it, last options gets control
              mode |= TREE|XTREE|SILENT;
              path = argv[i]+2;
         }
         else if(strnicmp(argv[i],"-l",2) == 0)
         {
             // this was first try, worked, limits on -x style extract
             // to a single path, change to -l 1/23/04
             mode &= ~XTREE; // clear it
             mode |= LPATH|SILENT;
             // force extract mode so don't need -x also
             path = argv[i]+2;
             path_len = strlen(path);
             if(path_len == 0)
                 continue;
             if(*(path+path_len-1) != '\\' &&
                 *(path+path_len-1) != '/')
                 path_len++; // treat trailing NUL as last slash
             for(rd = 0;rd<path_len;rd++)
               if(*(path+rd) == '\\' || *(path+rd) == '/')
                   *(path+rd) = 0;
             if(*path == 0 && path_len > 1)
             {
                path_len--;
                path++; // skip expected leading '/', but ignore if not there
             }
         }
         else if(strnicmp(argv[i],"-x",2) == 0)
         {
              mode |= EXTRACT|SILENT;
              if(*(argv[i]+2) != 0)
                   filter = argv[i]+2;
         }
         else if(*argv[i] == '@' &&
                 (rcnt = get_paths(argv[i]+1,paths,MAX_PATH)) > 0)
              mode |= TREE|SILENT;

       }

    if((fp = open(argv[1],O_BINARY|O_RDONLY)) == EOF)
       printf("\nfailed to open %s",argv[1]);
    else if(targ != 0L) // just do search and exit
       fnd_target(fp,targ,start,len);
    else if(mode & TAGS) // just display range of tags
       do_streams(fp,start,len);
    else
    {       
       if(off != 0L)
       {
          printf("\nSEEK to file position ");
          disp_foffset(off);
          printf(" for data recovery");
          if((FOFFSET)lseek(fp,off,SEEK_SET) != off)
          {
             printf(" - FAILED");
             off = 0L; // best guess, affects display of position
          }
       }
       while((rd = read(fp,buf,XPBLK_SZ)) > 4)
       {
           /* 
              12/03/04 think I now skip in most cases, see skip below
                 except DIRB.  Can conditionally enable skip there.
           */
           skip = 0;
           if((key=is_keyword((char *)buf)) > -1 && !(mode & SILENT))
           {
               printf("\n%4.4s found keyword at offset ",buf);
               disp_foffset(off);
           }

           // look at MTF checksum now that I know it exists
           if(key >= 0 && key <= MTF_MAX_BLK)
              i = hd_cksum((WORD *)buf,0x19); // block header cksum
           else if (key > 0) // assume stream header
              i = hd_cksum((WORD *)buf,0xa);
           if(key >= 0 && (mode & VERBOSE))
           {
              if(i !=0)
                  printf("\ncksum error in header");
           }

           if(key == TAPE && !(mode & SILENT))
           {
              do_tape();  // descriptive strings
           }
           else if(key == SSET)
           {
              if(!(mode & SILENT))
                  printf("  blk = %3lu",(unsigned long)(off/XPBLK_SZ));
              cset++; //increment current set 
              if(tset == 0 || cset == tset)
                 do_sset(cset);  // display descriptive strings
           }          
           else if(key == VOLB)
           {
                 dir_len = do_volb();
                 // don't care if truncated, although not likely, use string
                 if((tset == 0 || cset == tset) && mode & TREE)
                     rd = add_tree_node(&root, off, key); 
           }
           else if(key == DIRB)
           {
              /* both examples I've seen have had tag_head
                 followed by what looks like a disk specific structure
              */
              th = (MTF_DB_HDR *)buf;
              if(!(mode & SILENT))
                  printf(" 1st Stream: %4.4s",buf+th->var_sz);
              if(mode & LPATH && path != NULL) // its _MTF aware, and doing an extract
              {
                 if(match_path(path,path_len))
                     mode |= EXTRACT; // turn it on
                 else
                     mode &= ~EXTRACT; // turn off
              }
              skip = do_dirb(fp,off,mode,&dir_len);  // this can be big...
              if((tset == 0 || cset == tset) && mode & TREE)
              {
                  if(dir_len <= 0)
                      printf("\nskip truncated path");
                  else
                      rd = add_tree_node(&root, off, key); 
              }
           }
           else if(key == FILE_K)
           {
              tmode = mode;
              if(tset > 0 && tset != cset)
              {
                  tmode &= ~EXTRACT;  // don't extract, not current set
                  tmode |= SILENT;   // in fact, display nothing!
              }
              skip = do_file(fp,off,tmode);  // skips over file data, could extract
           }
           else if(key == TFDD)
           {
              if( (mode & CATALOG) && 
                   (tset == 0 || tset == cset)) // doing all or just this one
                   flag = 1;
              else
                   flag = 0; // parse region so skip over all of it

              skip = do_tfdd(fp,off,flag);
           }
           if(skip == LSEEK_ERR)
               printf("\nWaring skip error"); // 12/8/03 do nothing break; // fatal error
           if(skip > XPBLK_SZ)
              off += skip;
           else
              off += XPBLK_SZ; // always advances one block   

           /* did routine actually do the advance? do_file() only does if EXTRACT
              and if it did, did to skip ahead to next block boundry
           */
           if((skip = (FOFFSET)lseek(fp,0L,SEEK_CUR)) != off || skip % XPBLK_SZ != 0)
           {
              if(off % XPBLK_SZ)
                   off -= off % XPBLK_SZ;
              if((skip = (FOFFSET)lseek(fp,off,SEEK_SET)) != off)
              {
                   printf(" failed to skip ahead to next block");
                   exit(1);
              }
              
           }
           if(end != LSEEK_ERR && off >= end)
           {
               printf("\nparse terminated by end value from -j option at ");
               disp_foffset(off);
               break;
           }
       }
           
    }

    // TREE routines require a pass through the file to build the tree
    if(root != NULL)
    {
       if(mode & DTREE)
           disp_tree(root);
       if(mode & TREE && rcnt > 0)
           do_redirect(fp,root,paths,rcnt);
       else if(mode & XTREE && path != NULL)
       {
           i = strlen(path);
           i--;
           paths[0].term = *(path+i);
           if(i > 0 && (paths[0].term == DELIM || 
              paths[0].term == '*' || paths[0].term == '+'))
           {
              paths[0].path = path;
              *(path+i) = DELIM; // force for level detection
              paths[0].redirect = path+i+1; // empty string
              do_redirect(fp,root,paths,1); // allows recursive directory creation
           }
           else
               printf("\n-p option, invalid path %s",path);
/* note: following logic is a good test for path_extract()
   assumes path terminated with DELIM?
   have since replace with call to do_redirect() which I
   developed later to allow sub directory creation
   DIR_LIST  *dir;  // required allocation
   unsigned long fskipped=0;
           if((dir = find_path(root,path)) != NULL)
           {
              printf("\nDIRB record containing path at offset ");
              disp_foffset(dir->data_off);
              path_extract(fp,dir,&fskiped);
           }
*/
       }
       
    }
    fputc('\n',stdout); //cleanup for linux
    return(0);
}

