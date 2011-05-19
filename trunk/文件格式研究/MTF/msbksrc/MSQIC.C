/* msqic.c  *.QIC file reader for Win9x MSBackUp images
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

-----------------------------------------------------------------------------------
   History:
   Look at MSBackup fall of 2003  
   see also earlier work in qic80.c doing raw read of QIC80 trakker tapes
   with dd in Linux with ftape.

   Note below is abridged, see msqic8.c for full cumulative history.


9/30/03 copy qic80.c to this file and modify.
   Turns out there isn't a lot of similarity...
   Worked on it pretty much full time for a week

10/9/03 oh dear, win95 version has different VTBL structure
     and some differences in how to get to catalog.
     see notes in dcomp.txt.  Add display of major, minnor versions.
     Change how I get to the catalog (directory set). 

10/15/03 tweek for gcc, replace getchar() with fgetc(stdin)
     gets() with fgets()  add defines for strnicmp and strncmp
     #pragma pack(1) doesn't work with GCC under linux 2.4
     see changes in msqic.h
     add some logic to be sure a final '\n' printed for linux

11/29/03 current version on disk and in msqic.lzh is
     MSQIC    C          46,720  10-17-03  9:23a msqic.c
     works reasonably well with a consistent file, try
     adding -r recovery option.
     most of file depends on VTBL but can override some of it
     with -sd and -sc

     add free_cat_list() so can use cat_list routines to 
     grow each node during recovery.  Turned out didn't need this!

     The path is in the data region, but the file length isn't
     I guess at the length.  Seems to work with uncompressed
     WinME backup, messier with compressed and haven't dealt with
     Win95 issues

12/01/03 File up to 57kb, time to split again.
     create msqicrcv.c  for recovery and patch code.
     Ie do_recover() and related routines
        write_file() and next_data_sig()


12/03/03 expand -fs a little
     CAUTION need to verify this with multiple DRIVE WINME backups
     it calls find_seg() to get location of catalog for segment of interest

12/05/03 recompiled in linux, interesting -ss option was a little different.
     Changed include file to pack in both versions.
     Watch -t option, have email that says it blows up but can't reproduce.

12/13/03 ok Ralf send a sample archive.  Turns out it gives an error message
     and exits 'gracefully'.  Problem only seems to occur with a compressed
     archive because there are seg_head regions in the catalog.  His
     sample file contains more than one directory segment and the seg_head
     occurs in the middle of a file name.  Sigh.  Need to fix this
     and strip them out when totally decompress a file.  change 
     argument to do_decompress() to make this work

12/21/03 look at Ralf's 4_dir example of how my -t option was WRONG
     over a day on this!  Go to version 1.04 and archive as msqicv4.lzh

12/22/03 look into 4 Gb large file issues now.  Try define 4GB
     to force DWORD offsets via use of FOFFSET as my offset parameter
     ignore large file, 64GB issue for now as FAT32 doesn't support
     define _4GB at compile time to enable lseek4()
     see FOFFSET and LSEEK_ERR defines to handle this semi portably
     move tree routines and do_extract() to msqicrcv.c for space
     Bump version to 1.05

01/11/04 change arguments to do_extract() so pass ccat pointer
     force name in tree to MSDOS name from fix2 when MSDOS defined
     allows MSDOS to use MSDOS names, do_extract() doesn't care
     NM_LEN was 128, bump to 255
     Include Ralf's CYGWIN compiler defines
     
01/20/04 backup up current version with included DELIM  now
     defined in msqic.h in place of '\\' for root.
     Now try replacing with "ROOT"
    
01/22/04 allow '+' as optional terminator for -p option
     change args to do_extract()

02/03/04 minnor change to find_seg() appears to have a logic error.
     create msqic8.c with these changes.
     Now change to version 1.09 and go to all hex display and input
     as I have done in qicdcomp.c to prepare for possible 64 bit
     large file version.
     also change logic for -d so it uses start address of cseg_head
     rather than data following it so now consistent with -fs option

     oh ho, add start and end logic in get_vtbl() remove call to find_seg()
02/12/04 minnor correction in msqicrcv.c, the -r option did the
     timestamp/attrib preservation wrong cause I didn't save them before
     the global buffer was overwritten.
     Also add a bques() after getting the vtbl so one can attempt to
     continue.  Bump to version 1.10
04/28/04 add interactive options to update memory image
     of VTBL if it is known to be corrupt. See msqicrcv.c create_vtbl()
     Add interactive prompt in find_seg() to optional look at next chain.
     Add argument to bques() to allow a little customization, 
     ie returning -1 on ESC.  Bump to version 1.11

06/01/07 minnor update, bump version to 1.12 after copy current to msqic9.c
     see changes for Win95 compression I just detected from email
     add option -st95 for force to Win95 format, default is old behavior Win98 & ME
     add argument to find_seg() to handle this
     
----------------------

deleted inserts from QIC113G.pdf to get more memory for ED

*/

#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h> // define memset
#include <sys/types.h>
#include <sys/stat.h>   /* for open() defines */

#if  defined(MSDOS) || defined(_WIN32) 
# ifdef MSDOS
#  define OS_STR "MSDOS"
# else
#  define OS_STR "WIN32"
# endif
# include <memory.h>
# include <io.h>            // for lseek
#else
# ifdef unix
#  include <unistd.h>       // for read/write etc.
#  define strnicmp strncasecmp
#  define stricmp  strcasecmp
#  ifdef __CYGWIN__
#    define OS_STR "CYGWIN" 
#else
#    define OS_STR "LINUX"
#  endif
# endif
#endif

#ifndef OS_STR
#  error Unknown build environment.
#endif

#include "msqic.h"


#define BLK_SZ 1024 
BYTE buf[BLK_SZ];  // used by most read routines. Assumes
                   // all blocks smaller than this, checks if correct


#define BASEYR 1970 // for Unix or 1904 for Mac  just to be different!
unsigned char mondays[] = {31,28,31,30,31,30,31,31,31,30,31,31};

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

// see pp 204 in runtime C lib ref for my C/C++ ref (used only in testing)
void disp_dosdt(unsigned long date)
{
   int mon,day,yr,hour,min,sec;
   sec = 2 *(date & 0x1f);
   date = date >> 5;
   min = (date & 0x3f);
   date = date >> 6;
   hour = date & 0x1f;
   date = date >> 5;
   day = date & 0x1f;
   date = date >> 5;
   mon = date & 0xf;
   date = date >> 4;
   yr = date+1980; // high order 7 bits
   printf("%02d/%02d/%04d  %02d:%02d:%02d",mon+1,day,yr,hour,min,sec);
}

/* may be an off by one day in below, but seems to work
   should be able to do more directly!
*/
void disp_datetime(unsigned long date)
{
    unsigned short yr=BASEYR,mon=0,day,hour,min,sec;
    char lpyr;
    sec = date % 60;
    date /=60;
    min = date % 60;
    date /= 60;
    hour = date % 24;
    date /= 24;
    do {
        if((yr % 100) == 0 || (yr % 4) != 0)
        {
           day = 365; // not a leap year
           lpyr=0;
        }
        else 
        {
           day = 366; // is a leap year
           lpyr = 1;
        }
        if(date > day)
        {
           yr++;
           date -= day;
        }
    } while (date > day);
    day = date;

    mondays[1] += lpyr;
    while(mon < 12)
    {
       if(mondays[mon] >= day)
         break;
       else
         day -= mondays[mon++];
    }
    mondays[1] -= lpyr;
    printf("%02d/%02d/%04d  %02d:%02d:%02d",mon+1,day,yr,hour,min,sec);
}

/* try to reverse process above.  Currently only called from
   msqicrcv.c  create_vtbl()

   assumes buf[] delimited with '/' and has format
   "mon/day/year"

   returns seconds elapsed since 1970 till date in string

*/
unsigned long mk_date(char *buf)
{
    unsigned long date=0;
    int i,mon=0,day=0,yr=0;
    char *ptr=buf;
    if(sscanf(ptr,"%d",&mon) == 1  && 
       (ptr = strchr(ptr,'/')) != NULL &&
       sscanf(ptr+1,"%d",&day) == 1  && 
       (ptr = strchr(ptr+1,'/')) != NULL &&
       sscanf(ptr+1,"%d",&yr) == 1)
    {
       if(yr < 70) // probably only entered 2 digits
           yr+= 2000;
       if(mon > 0)
          mon--; // make an index, ie zero based
       if(yr >= BASEYR)
       {
           date = (yr - BASEYR) * 365; // total normal year days
           date += (yr - BASEYR)/4;
           if(yr >= 2000)
              date--; // this adds number of leap years, ie extra days
           for(i=0;i<mon;i++)
              date += mondays[i];
           date += day;  // add in day of current month
           date *= (24L * 60 * 60); // *= hours in a day              
       }        
       
    }
    return(date);
}


/* return bytes read on succes, else < 0
   assume appropriate seek has been done to get to position
   root record is a bit screwy.... add logic to allow partial read
   of 2nd fixed region

   10/7/03 remove flag parmater, I'm not using, put everything
   in struct dir_blk

   11/31/03 change params, adding a mode and buffer info so
   can call stand alone, esp from do_recover()
   add some logic to test if W95 or in catalog, ie W98 bit not set
   in W98 the rec_len field in data region is not set for some reason.

   12/14/03 attempt to add compressed file logic where track
   location of next cseg_head in ptr->pseg buffer for compressed files
   this is all done in cat_read() routine which replaces original read()
      This strips the embedded cseg_head structures from a
      compressed archives catalog region
*/
int cat_read(int fp,struct cseg_head *pseg,BYTE *buf,WORD sz)
{
    int i,rd,trd = 0,suc=0;
    if(pseg == NULL) // just do a normal read operation (old behavior)
        trd = read(fp,buf,sz);
    else
    while(suc == 0 && trd < sz)
    {
       rd = 0;
       if(pseg->seg_sz == 0)  // read next seg head
       {
          if((i = read(fp,pseg,sizeof(struct cseg_head))) != sizeof(struct cseg_head))
             break; // error reading next seg head
          pseg->seg_sz &= ~RAW_SEG; // clear raw bit
       }
       else if(pseg->seg_sz < sz)
       {     // must read last bytes in this segment
            if((rd = read(fp,buf+trd,pseg->seg_sz)) != pseg->seg_sz)
               suc++;
       }
       else if ((rd = read(fp,buf+trd,sz-trd)) != sz-trd)
          suc++;
       if(rd > 0)
       {
           trd += rd;
           if(pseg->seg_sz < rd)
              pseg->seg_sz = 0; // corrected, no longer seems to occur
           pseg->seg_sz -= rd;          
       }
    }
    return(trd);
}

// returns bytes read on success and ptr->err == 0
// or a value < 0 indicating error and ptr->err == bytes read
// on success *ptr is initialized with directory fields
int  next_dir(int fp,struct dir_blk *ptr, BYTE *buf,WORD bsz,BYTE mode)
{
    int i,len,rd=0,suc=0;
    char tst;
    ptr->fix1 = NULL; ptr->fix2 = NULL;
    ptr->nm1 = ptr->nm2 = NULL;
    ptr->err = 0;

    if((i=cat_read(fp,ptr->pseg,buf,sizeof(struct ms_dir_fixed))) !=
            sizeof(struct ms_dir_fixed))
    {
        printf("\nerror reading 1st fixed region");
        suc = -2;
    }
    else 
    {
        ptr->fix1 = (struct ms_dir_fixed *)buf;
        rd +=i;
        i = 0; // name can be empty for root
        tst = 0; // assume false, ignores rec_len
        if(!(mode & W98) && // in catalog or W95 data mode, rec_len is valid
           ptr->fix1->nm_len +rd > ptr->fix1->rec_len +2)
            tst = 1;
        if(ptr->fix1->nm_len > 0 && (ptr->fix1->nm_len > bsz - rd || tst ||
          (i=cat_read(fp,ptr->pseg,buf+rd,ptr->fix1->nm_len)) != ptr->fix1->nm_len) )
        {
           printf("\nerror reading 1st fixed region");
           suc = -3;
        }
        else
        {
           if( i > 0)
           {
              ptr->nm1 = buf+rd;
              rd +=i;
           }
           len = sizeof(struct ms_dir_fixed2);
           /* prior versions just warned if couldn't read all
              of fixed2, but its now fatal error, don't know where we are!
           */
           if(len+rd > bsz || (i=cat_read(fp,ptr->pseg,buf+rd,(WORD)len)) != len)
           {
               printf("\nerror reading 2nd fixed region");
               suc = -4;
           }
           else 
           {
              ptr->fix2 = (struct ms_dir_fixed2 *)(buf + rd);
              rd += i;
              i = 0; // name can be empty for root
              if(ptr->fix2->nm_len > 0)
              {
                 tst = 0; // assume false, ignores rec_len
                 if(!(mode & W98) && // in catalog or W95 data mode, rec_len is valid
                    ptr->fix2->nm_len +rd > ptr->fix1->rec_len +2)
                         tst = 1;

                  if ( ptr->fix2->nm_len > bsz - rd || tst ||
                     (i=cat_read(fp,ptr->pseg,buf+rd,ptr->fix2->nm_len)) != ptr->fix2->nm_len)
                  {
                     printf("\nerror reading 2nd name");
                     suc = -5;
                  }
                  else if(i > 0)
                  {
                     ptr->nm2 = buf+rd;
                     rd +=i;
                  }
              }
           }
        }
           
    }
    if(suc == 0)
        suc = rd;
    else
        ptr->err = suc; // save it here for fun
    return(suc); 
}



#define RD_SZ (BLK_SZ -3)
int do_search(int fp,DWORD targ)
{
   int len,i;
   FOFFSET offset;
   DWORD *lptr;
   printf("\nsearching file for occurances of 0x%lx",targ);
    if((offset = lseek(fp,0L,SEEK_CUR)) < LSEEK_ERR || read(fp,buf,3) != 3)
       return(-1);
    do {
       if((len = read(fp,buf+3,RD_SZ)) != RD_SZ)
           return(-1);
       for(i=0;i<len;i++)
       {
         lptr = (DWORD *)(buf+i);
         if(*lptr == targ)
           printf("\nhit at offset %lx",offset+i);
       }
       for(i = 0;i<3;i++)
         buf[i] = buf[i +len]; // move end to begining and repeat
       offset += len;
    } while(len == RD_SZ);
    return(0);
}


/* flag defines used in main, moved to msqic.h
#define EXTRACT 2
#define TREE   4  // display catalog as tree structure
*/

unsigned offs[] = {0x2,0x2d,0x35,0};

char *flagbits[] = {
   "Vendor specific volume",
   "Volume spans multiple cartidges",
   "File sets written without verification",
   "Reserved (should not occur)",
   "Compressed data segment spaning",
   "File Set Directory follow data section"
};
   
char *OStype[] = {
   "Extended",
   "DOS Basic",
   "Unix",
   "OS/2",
   "Novell Netware",
   "Windows NT",
   "DOS Extended",
   "Windows 95"
};

char *dcompfnm = "dcomp.out";


void disp_vtbl(struct qic_vtbl *vtbl)
{
    int i,rd;
         printf("\nLabel: %.44s  \nVTBL volume contains %lu logical segments",
              vtbl->desc,vtbl->nseg);
// oh dear added 10/4, thought would be easy, but need to fudge?  why?
// 63072000 would be two years in seconds + 691200 is 8 days???
// above is value of VTBL_DATE_FUDGE also used in msqicrcv.c:create_vtbl()
         printf("\ncreated (aprox): "); disp_datetime(vtbl->date - VTBL_DATE_FUDGE);
//         printf("\ncreated: "); disp_dosdt(vtbl->date);  way off yr=2519

         printf("\nflag 0x%x:",vtbl->flag);
         for(i=0,rd=1;i<5;i++)
         {
             if(rd & vtbl->flag)
                printf("\n%s",flagbits[i]);
             rd = rd << 1;
         }
         if((vtbl->flag & 1) == 0) // generic, not vendor specific
         {  
            // fields after flag not valid if vendor specific

            // ignore quad word, assume vtbl->dataSz[1] == 0
            printf("\nversion: %0x:%0x",vtbl->rev_major,vtbl->rev_minor);
            printf("\ndir size 0x%lx data size 0x%lx",
                vtbl->dirSz, vtbl->dataSz[0]);
            printf("\nQFA physical start block 0x%lx end block 0x%lx",
               vtbl->start, vtbl->end );
            printf("\ncompression byte 0x%x",vtbl->comp);
            if(vtbl->comp & 0x80)
               printf("\nCompression used, type 0x%x",vtbl->comp & 0x3f);
            if(vtbl->OStype < 8)
               printf("\nOS type: %s",OStype[vtbl->OStype]);
         }
}

/* 1/16/04 update so ctrl controls display mode and
   if it gets cur_pos from current file pos
   or seeks to end param passed (note this is called from get_vtbl()

   also change to FOFFSET and LSEEK_ERR for 4GB option
   I don't think *end = -1 was ever used.
   now done with ctrl & FIND_POS
   2/2/04 see avik.c for alternative search handles Avik's file
   2/29/04 If ctrl & DISPLAY add interactive prompt to allow skipping 
      ahead to next chain.  Useful in multi-volume file.
   6/01/07 add arg ver to allow WIN95 format to be forced
      add min_seg_sz test for WIN95
*/

int find_seg(int fp,char ctrl,FOFFSET *end,BYTE ver)
{   
     char hit = 0,ques=0;
     int i,tstsz,min_seg_sz,rd,sz,cnt=SEG_SZ/BLK_SZ +2; // 2 blocks more than a segment
     FOFFSET cur_pos,tst_pos,adv;   
     struct cseg_head cseg,*pcseg;
     tstsz = SEG_SZ - sizeof(struct cseg_head); // bytes in a full segment
     min_seg_sz = tstsz - 0x20;  // wild guess  WIN95 uses actual bytes used
     if(ctrl & FIND_POS) // equivalent to *end = -1 in prior version
         cur_pos = lseek(fp,0L,SEEK_CUR); // where are we now
     else
         cur_pos = *end; // force position
     if(cur_pos == LSEEK_ERR)
         return(LSEEK_ERR);
     if((tst_pos = lseek(fp,0L,SEEK_END)) != LSEEK_ERR && // found EOF
        ((FOFFSET)cnt * BLK_SZ) +SEG_SZ > tst_pos)
        cnt = (tst_pos - SEG_SZ)/BLK_SZ; // remainder fits in a segment
     // won't try if there isn't more than one segment in file...
         
     tst_pos = cur_pos; // will still duplicate old logic if no hit
     while(hit==0 && cnt-- > 0 && 
           (FOFFSET)lseek(fp,tst_pos,SEEK_SET) == tst_pos &&
           (rd = read(fp,buf,BLK_SZ)) == BLK_SZ)
     {
         for(i = 0;i<BLK_SZ-sizeof(struct cseg_head) && hit == 0;i++)
         {
              pcseg = (struct cseg_head *)(buf+i);
              // possible hit if either this seg is full, or its 1st seg
              // can still miss if entry point is just ahead of last seg
              if((sz = pcseg->seg_sz & ~RAW_SEG) == tstsz || 
                  ((ver&WIN_MASK) == WIN95 && sz <= tstsz && sz >= min_seg_sz) || // less restrictive
                  (pcseg->cum_sz == 0 && pcseg->cum_sz_hi == 0 && sz > 0) ) 
              {
                  adv = tst_pos + SEG_SZ +i;  // should be ok WIN95 & WINME
/*
                  adv = tst_pos+i;
     6/14/07 this must be wrong!  think I just added, but not right, above already did most
                  if(ver == WIN95) // new 6/1/07 else default to prior Win98
                     adv += SEG_SZ;
                  else
                     adv += sz+sizeof(struct cseg_head);
*/
              // try a seek to next segment header
                  if((FOFFSET)lseek(fp,adv,SEEK_SET) != adv ||
                     (rd = read(fp,&cseg,sizeof(cseg))) != sizeof(cseg))
                     hit--; // fatal error testing for chain
                  else if((cseg.cum_sz == 0 && cseg.cum_sz_hi == 0 && pcseg->seg_sz == 0) ||
                      // special case below if 1st uncompressed seg, has cksum?
                      (pcseg->seg_sz & RAW_SEG && pcseg->cum_sz == 0 && sz < 0x7F6 &&
                       cseg.cum_sz & 0xFFFF0000 == 0 && cseg.cum_sz_hi == 0 &&
                       cseg.seg_sz == 0) ||

                      // last is a lose test, is cum_sz consistent?
                          (cseg.cum_sz >= pcseg->cum_sz + sz &&
                           cseg.cum_sz_hi == 0)) // asssume <= 4GB
                     hit++;
                     cur_pos = tst_pos+i;
              }
         }
         tst_pos += BLK_SZ -sizeof(struct cseg_head);
     }
     if(ctrl & DISPLAY && hit > 0)
     {
         printf("\nFound what looks like a valid segment chain");
         *end = cur_pos; // set start point if interactive
     }
     cnt = 0;
     while((FOFFSET)lseek(fp,cur_pos,SEEK_SET) == cur_pos &&
           (rd = read(fp,&cseg,sizeof(cseg))) == sizeof(cseg))
     {
         cnt++;
         sz = cseg.seg_sz & ~RAW_SEG;
         if(ctrl & DISPLAY)
         {
              printf("\n%3d: @ 0x%lx  cum size = 0x%lx  segment size 0x%x",
                     cnt,cur_pos,cseg.cum_sz,sz);
              if(cseg.seg_sz & RAW_SEG)
                   printf(" - not compressed");
              if(cseg.seg_sz == 0)
              {
                  printf("\nend of current list, try to step into next? (Y/N) ");
                  ques = bques(0); // only do interactive logic in DISPLAY mode
              }
         }
         cur_pos += SEG_SZ;  // change 6/01/07 should work for all versions
         if(sz == 0)
         {
              if(ques == 1) // attempt to advance through catalog to next segment
              {
                 tst_pos = (cur_pos - *end) % SEG_SZ;
                 if(tst_pos != 0)
                     cur_pos += SEG_SZ - tst_pos;
                 // used compressed flag below, must be if has cseg_heads!
                 adv = get_dir_len(fp,1,cur_pos);
                 printf("\nCatalog expected at 0x%lx",cur_pos);
                 if(adv != LSEEK_ERR)
                 {
                     printf("  len 0x%ld",adv);
                     i = adv / SEG_SZ; // # of segments
                     tst_pos = adv % SEG_SZ;
                     if(tst_pos != 0)
                     {
                         i++;
                         adv += SEG_SZ - tst_pos; // next boundry
                     }
                     // catalog always an even # of full segments
                     cur_pos +=  i * SEG_SZ; 
                     printf("\n\nAdvance to 0x%lx for next segment chain",cur_pos);
                 }
                 else
                 {
                     printf("  can't determine length");
                     ques = 0;
                 }

              }
              if(ques == 0)
              {
                 *end = cur_pos; // return where we are
                 return(cnt); // # of segments
              }
         }
         rd = 0;
     }
     if(ctrl & DISPLAY)
        printf("\nnever found an end segment\n");
     return(LSEEK_ERR); 
}

char  *VTBL_TAG = "VTBL",
      *MDID_TAG = "MDID";

/* side effect leave positioned where I think data starts
   note the WinME logic is messy as must call find_seg() to
   locate where each compressed data region ends, then round
   up to next segment boundry to find start of its catalog
   2/4/04 oh how use start and end -3 for zero based 
   index to start data and start catalog

*/

int get_vtbl(int fp,struct vtbl_ver *cur_vtbl)
{
    char query[30];
    long tag,*lptr = (DWORD *)&cur_vtbl->vtbl;
    int suc = 0,cnt = 0,i,rd,trd=0;
    cur_vtbl->ver = WIN95; // default

    while(read(fp,&tag,sizeof(long)) == sizeof(long) &&
         tag == *((long *)VTBL_TAG) && 
         (rd = read(fp,lptr+1,sizeof(struct qic_vtbl) - sizeof(DWORD))) ==
                     sizeof(struct qic_vtbl) - sizeof(DWORD))
    {
         *lptr = tag; // insert at start of vtbl
         trd += rd+sizeof(DWORD);
         cnt++;
         if(cnt == 1)
            printf("\n  %.44s",cur_vtbl->vtbl.desc);
         // Win95 has no sdrv, for winME they are valid
         if(cur_vtbl->vtbl.sdrv[0] > 0)
            printf("\n%d:  %c: %.16s",cnt,cur_vtbl->vtbl.ldev-1+'A',
                     cur_vtbl->vtbl.sdrv);  
         else printf("\n%d: logical dev 0x%x",cnt,cur_vtbl->vtbl.ldev);
        
    }
    if(trd == 0 || trd != cnt * sizeof(struct qic_vtbl))
    {
        printf("\nerror reading VTBL region");
        suc++;
    }
    if(tag == *((long *)MDID_TAG) )
    {
       printf("\n%s tag found at end of VTLB region => WinME format",MDID_TAG);
       cur_vtbl->ver = WINME;
       cur_vtbl->database = 128;  // extra record at end
    }
    else
       cur_vtbl->database = 0;

    cur_vtbl->cnt = cnt; 
    if((cur_vtbl->flen = (DWORD)lseek(fp,0L,SEEK_END)) == LSEEK_ERR || 
        cur_vtbl->flen < SEG_SZ)
    {
         printf("\nerror finding file length");
         suc++;
    }

 
    // need additional logic for multiple embedded archives
    // note only one copy of VTBL at start of file
    if(cnt > 1)  // WinME generates a VTBL for each drive accessed!
    {
         printf("\nSelect one of archives above (1 - %d): ",cnt);
         fgets(query,10,stdin);
         if(sscanf(query,"%d",&i) != 1 ||
              (cur_vtbl->ndx = (BYTE) i) < 1 || cur_vtbl->ndx > cnt)
         {
              printf("\nabort - invalid choice");
              suc++;
         }
         else  // must reread selected entry
         {
              i--; // above is 1 base, want 0 based
              // seek to next vtbl entry and read it
              if(lseek(fp,128L*i,SEEK_SET) != 128L*i ||
                 (rd = read(fp,&cur_vtbl->vtbl,sizeof(struct qic_vtbl))) !=
                     sizeof(struct qic_vtbl))
              {
                   suc++;
                printf("\nfailed to reread desired VTBL");
              }
         }
    }
    else 
         cur_vtbl->ndx = cnt;

    if(suc == 0)
    {
       if((cur_vtbl->ver & WIN_MASK) == WINME && 
          (cur_vtbl->vtbl.start < 3 || cur_vtbl->vtbl.end < 3))
       {
           printf("\nInvalid QFA block #'s");
           if(cnt > 1)
              suc++; // fatal error
       }
    }

    if(suc == 0)
    {
         // all are relative to end VTBL region
         cur_vtbl->database += cnt * 128;
         cur_vtbl->dirbase = cur_vtbl->database;
         if(cnt > 1 && (cur_vtbl->ver & WIN_MASK)== WINME)
         {  // may have multiple volumes
            cur_vtbl->database += 
              (FOFFSET)(cur_vtbl->vtbl.start -3) * SEG_SZ;
         
             cur_vtbl->dirbase += 
                (FOFFSET)(cur_vtbl->vtbl.end -3) * SEG_SZ;
         }
         else // assume one contiguous volume to EOF
         {
            i = cur_vtbl->vtbl.dirSz/SEG_SZ;
            if(cur_vtbl->vtbl.dirSz % SEG_SZ != 0)
               i++;
            cur_vtbl->dirbase = cur_vtbl->flen - (FOFFSET)i * SEG_SZ;
         }
    }
    else // try fudging, set data to full range of file
    {
         cur_vtbl->database = 0;
         cur_vtbl->dirbase = cur_vtbl->flen;
    }
    printf("\nstart data 0x%lx  start dir 0x%lx",
                   cur_vtbl->database,cur_vtbl->dirbase);


    if((DWORD)lseek(fp,cur_vtbl->database,SEEK_SET) !=cur_vtbl->database)
    {
        printf("\nfailed to seek to start of data region %lx",cur_vtbl->database);
        suc++;
    }

    return(suc);
}

/* adjust to handle multiple questions,
   unless in raw mode, doesn't return without CR
   so need to eat trailing control char
   4/28/04 add esc argument
*/
int bques(char esc)
{
   int ret=0;
   char  ch;
   // Enter on pc returns 0x10, exit loop on any control key
   while((ch = fgetc(stdin)) > ' ' && ch != esc) 
   {
      if(ch == 'y' || ch == 'Y')
         ret = 1;
      else
         ret = 0;
   }
   if(ch == esc)
      ret = -1;
   return(ret);
}


#define NM_LEN 255 // size preallocated buffer for node name

PATH_ELEM paths[MAX_PATH];
 
// the tstbackup.qic files directory set starts at off = 0x2B90A
// have auto detect working now, can override with -s178442
int main(int argc,char *argv[])
{
    extern int fin,fout; // decompression globals
    int fo=EOF,fp=EOF,i,cnt = 0,dcnt=0;
    int nm_len=0,targetlen,rcnt=0,rd,suc=1;
    long dir_len=0,*lptr;
    FOFFSET off = 0,datapos,l; 
    DWORD fdata=0;
    char *target=NULL,dch,ch,flag=0,tmode=0,doexit=0;  
    char name[NM_LEN+1];
    struct cseg_head seg;
    struct dir_blk dir;
    struct vtbl_ver cur_vtbl;
    // root,current,temp, next child
    CAT_LIST *root=NULL,*ccat,*tcat,*ncat=NULL;
    cur_vtbl.vtbl.dataSz[0] = 0; // used in test for validity below

    printf("\nMSQIC Ver 1.12  compiled for %s",OS_STR);
#ifdef _4GB
    printf("\n   this version compiled for 4 Gb file access");
#endif
    printf("\nCopyright (C) 2003 William T. Kranz");
    printf("\nMSQIC comes with ABSOLUTELY NO WARRANTY");
    printf(
"\nFree software distributed under the terms of the GNU General Public license");
    printf(
"\nSee http://www.gnu.org/licenses/gpl.html for license information");
    printf(
"\nCheck http://www.fpns.net/willy/msbackup.htm for Updates & Documentation\n");
    // debug aid check structure sizes
    if(argc > 1 && strnicmp(argv[1],"-ss",3) == 0)
    {
        printf("\nstructure sizes");
        printf("\nsizeof(struct qic_vtbl) = %d",sizeof(struct qic_vtbl));
        printf("\nsizeof(struct cseg_head) = %d",sizeof(struct cseg_head));
        printf("\nsizeof(struct ms_dir_fixed) = %d",sizeof(struct ms_dir_fixed));
        printf("\nsizeof(struct ms_dir_fixed2) = %d",sizeof(struct ms_dir_fixed2));
        fputc('\n',stdout);
        exit(0);
    }
    if(argc < 2)
    {

         printf(
"\nmsqic <file> [@cmd] [-p] [-x<nm>] [-v] [-t] [-s{c|d}#] [-f{d|e|s}] [-d] [-r]");
         printf("\n@cmd to extract directories based on cmd file");
         printf("\n-p<path> extract ALL files from ONE path in tree structure");
         printf("\n-x to extract a file, nm, using paths in tree structure");
         printf("\n-v just display VTBL and exit");
         printf("\n-t[ds] to display catalog as tree, d=> dir only, s=> with segment info");
         printf("\n-fd find file id 0x%lx in data",DAT_SIG);
         printf("\n-fe find file id 0x%lx in data",EDAT_SIG);
         printf("\n-fs find & display compressed file segments");
         printf("\n-sc# force start catalog (directory set) at hex offset");
         printf("\n-sd# force start data region at hex offset");
         printf("\n-st95  force Win95 decompression decode, default is Win98 & ME");
         //        -sv<filenm>  is hidden, update VTBL from data file
         printf("\n-D to decompress archive write output to %s",dcompfnm);
         printf("\n-d##[:#] to decompress segment(s) starting at hex offset ## in file");
         printf("\n    use optional hex :cnt to decompress cnt contiguous segments");
         printf("\n-r[filter] attempt raw file data recovery, use -sd# to force data region start");
         printf("\n  use optional filter string, ie *.txt to limit hits");
         doexit++;                  
    }
    else if ((fp = open(argv[1],O_BINARY|O_RDONLY)) == EOF)
    {
         printf("\nfailed to open %s",argv[1]);
         doexit++;
    }
    else if((suc=get_vtbl(fp,&cur_vtbl)) != 0)
    {
        printf("\nfailed to find a valid vtbl, try using -sc and -sd?");
    // above positions at start data if found
        printf("\nContinue with invalid VTBL? (Y/N) ");
        if(bques(0) == 1)
        {
           suc = create_vtbl(&cur_vtbl); // allow to continue
        }
    }


    for(i=2;doexit == 0 && fp != EOF && i<argc;i++)
        if(strnicmp(argv[i],"-x",2) == 0)
        {
            flag |= (EXTRACT|TREE); // surpress raw catalog
            target = argv[i]+2;
            targetlen=strlen(target);
        }
        else if(*argv[i] == '@' && (rcnt = get_paths(argv[i]+1,paths,MAX_PATH)) > 0)
        {
            flag |= TREE;
        }
        // don't overwrite @, give it precedence
        else if (strnicmp(argv[i],"-p",2) == 0 && rcnt <= 0 &&
            (rd = strlen(argv[i]+2)) > 0)
        {   // extract a directory to current directory
            // set up for a call to do_redirect() with one path
            paths[0].term = *(argv[i]+rd+1);
            if(paths[0].term != DELIM && paths[0].term != '*' && paths[0].term != '+')
            {
                printf("\nerror in -p option, path must end in %c, '*', or '+'",DELIM);
                doexit++; // exit so see error message
            }
            else
            {

                if((paths[0].path = malloc(rd)) == NULL)
                {
                   printf("\nfailed to allocate path for -p option");
                   doexit++; // exit so see error message
                }
                else
                {
                   rd--;
                   strncpy(paths[0].path,argv[i]+2,rd);
                   *(paths[0].path+rd) = 0; // terminator, del DELIM
                   paths[0].redirect = paths[0].path+rd; // current dir => empty redirect
                   rcnt = 1;
                   flag |= TREE; // tree is required
                }                               
            }
        } 
        else if(strnicmp(argv[i],"-t",2) == 0)
        {
            flag |= TREE;
            // may have two conditional arguments
            for(ch=2;ch<5 && argv[i][(int)ch] != 0;ch++)
            {
              dch=argv[i][(int)ch];
              if( dch == 'd' || dch == 'D')
                   tmode |= DIRONLY; // show dir only
              else if( dch == 's' || dch == 'S')
                   tmode |= S_SEGS; // show segs
            }
        }
        else if(strnicmp(argv[i],"-sc",3) == 0 &&
            sscanf(argv[i]+3,"%lx",&l) == 1)
        {
             cur_vtbl.dirbase = l;
        }
        else if(strnicmp(argv[i],"-sd",3) == 0 &&
            sscanf(argv[i]+3,"%lx",&l) == 1)
        {
             if((FOFFSET)lseek(fp,l,SEEK_SET) != l)
                 printf("\nfailed to seek to data start at %lx",l);
             else
                 printf("\nfile pointer at data start %lx",l);
             cur_vtbl.database = l;
        }
        else if(strnicmp(argv[i],"-st95",5) == 0 )
             cur_vtbl.ver = WIN95;  // force WIN95 flag for decompression
        else if(strnicmp(argv[i],"-sv",3) == 0)
        {
            if(fp != EOF)
              close(fp);
            write_vtbl(argv[1],argv[i]+3);
            doexit++;
        }
        else if(strnicmp(argv[i],"-fd",3) == 0)
        {
            do_search(fp,DAT_SIG);
            doexit++;
        }
        else if(strnicmp(argv[i],"-fe",3) == 0)
        {
            do_search(fp,EDAT_SIG);
            doexit++;
        }
        else if(strnicmp(argv[i],"-fs",3) == 0)
        {
            if((cur_vtbl.vtbl.comp & 0x80) == 0)
            {
                printf("\nWarning, this file not compressed, continue (Y/N) ");
                if(bques(0) == 0) // No
                   doexit++;
            }
            if(!doexit)
            {
               l = cur_vtbl.database; // use currently assigned database
               find_seg(fp,DISPLAY,&l,cur_vtbl.ver); // display compressed segment list
               doexit++;
            }
        }
        else if(strnicmp(argv[i],"-d",2) == 0)
        {
            if(!(cur_vtbl.vtbl.comp & 0x80))
            {
               printf("\nVTBL says file data was not compressed! force it (Y/N)? ");
               if(bques(0) != 1)
                   exit(-1);
            }
            if ((fo=open(dcompfnm,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,
                        S_IREAD|S_IWRITE)) == EOF)
                printf("\nfailed to open decompress output file");
            else if(*(argv[i]+1) == 'D') // do entire file
            {
              do_decompress(fp,fo,&cur_vtbl);
              doexit++;
            }
            else if(*(argv[i]+1) == 'd')
            {
               // Note argument should be address AFTER cseg_head
               if(strlen(argv[i]) > 2 && sscanf(argv[i]+2,"%lx",&l) == 1)
               {
                  // for DATA RECOVERY check for a length qualifier 
                  if((target = strchr(argv[i]+2,':')) != NULL &&
                     sscanf(target+1,"%x",&cnt) == 1 && cnt > 0)
                         printf(
                  "\n Attempt to read 0x%x continguous segments",cnt);
                  else if(target != NULL)
                  {
                      printf("\nsyntax error, ignore :# format");
                      target = NULL;
                  }
                  rd = sizeof(struct cseg_head);

                  if((FOFFSET)lseek(fp,l,SEEK_SET) != l)
                  {
                     printf("\nseek to %lx failed",l);
                     doexit++;
                  }
                  if(target == NULL)
                  {
                      cnt = 1;
                      l += rd; // skip cseg_head, go right to data 
                  }
                  fin = fp; fout = fo; // set globals
                  while(!doexit && cnt > 0)
                  {
                     if(target != NULL)
                     {
                        if(read(fin,&seg,rd) != rd)
                        {
                           printf("\nabort, failed to read segment header");
                           break;
                         }
                         else if(seg.seg_sz == 0)
                         {
                            printf("\nend of segment header chain");
                            break;
                         }
                         // use ch as compressed flag
                         if(seg.seg_sz & RAW_SEG)
                         {
                            ch = 0;
                            seg.seg_sz &= ~RAW_SEG; // clear it
                         }
                         else
                            ch = 1; // is compressed
                         printf("\n\nseg start %lx  seg size %x",
                             l,seg.seg_sz);
                         if(!ch) // don't decompress this one!
                         {
                  printf("\nRaw copy of uncompressed segment starting at %lx",l);
                            // do a raw copy of the segment, ignore return....
                            copy_region((long)(seg.seg_sz));
                         }
                         else
                            decomp_seg(); // repeat for cnt segs with no checks
                         // new 6/1/07
                            l += SEG_SZ;
                         // was WINME specific:  l += seg.seg_sz+rd;  
                     }
                     else
                           decomp_seg(); // let it rip ONCE with no checks
                     if(--cnt > 0 && (FOFFSET)lseek(fin,l,SEEK_SET) != l)
                     {
                        printf("\nerror seeking to next segment header at %lx",l);
                        break;
                     }
                  }
                  doexit++;
               }

            }
        }
        else if(strnicmp(argv[i],"-r",2) == 0)
        {
            target = argv[i]+2;
            if(strlen(target) == 0)
                target = NULL;
            // flen set via lseek() in get_vtbl() its valid even if vtbl isn't
            if(cur_vtbl.dirbase > cur_vtbl.flen)
                cur_vtbl.dirbase = cur_vtbl.flen;
            do_recover(fp,cur_vtbl.database,cur_vtbl.dirbase,buf,BLK_SZ,target);
            doexit++; // just display VTBL
        }
        else if(strnicmp(argv[i],"-v",2) == 0)
        {
            disp_vtbl(&cur_vtbl.vtbl);
            doexit++; // just display VTBL
        }

    if(suc || doexit)
    {
#ifdef unix
         printf("\n"); // cleanup for unix
#endif
         exit(suc);
    }

    // we have parsed command line args, and need to process catalog
    // move to start dir region
    else if((DWORD)lseek(fp,cur_vtbl.dirbase,SEEK_SET) !=cur_vtbl.dirbase)
    {
        printf("\nfailed to seek to start of catalog at %lx\n\n",cur_vtbl.dirbase);
        suc++;
    }
    else   // parse directory display files, or build tree
    {
         // cseg_head values are relative to start data region.
         datapos = 0; // initialize to start data region
         // developed logic for WinME where root is not named
         ccat = new_cat_list(NULL,"ROOT",datapos,NULL); // create root node
         printf("\ndirectory display starting at offset %lx",cur_vtbl.dirbase);
         // faults out at end of list terminate before then!

         if( cur_vtbl.vtbl.comp & 0x80) // compressed file
         {
              dir.pseg = &seg; // uncompressed flag
              memset(&seg,0,sizeof(struct cseg_head));
         }
         else
              dir.pseg = NULL; // uncompressed flag
         while((suc = next_dir(fp,&dir,buf,BLK_SZ,0)) > 0)
         {
             if(dir.fix1 == NULL || dir.fix1->rec_len == 0)
             {
                printf("\nbig oops, must be end of directory");
                break;
             }
             if((dir.fix1->flag & SUBDIR)) 
             {
                 dch = 'D';
                 dcnt++;
             }
             else
             {
                 dch = ' ';
                 cnt++;
                 fdata +=dir.fix1->file_len;
             }
             dir_len += suc;
             off += suc;
             if(suc != dir.fix1->rec_len+2) // len is for rest of rec
                printf("\nwarning rec_len = %d != bytes read %d",
                        dir.fix1->rec_len,suc);

             
             // rec_len is length of rest of record without its own WORD length
             datapos += dir.fix1->rec_len + 2 + sizeof(long)+ // DAT_SIG
                        dir.fix1->path_len; // path data in data section
             // if sub dir pointing to next DAT_SIG in next subdir
             // if its a file advance a little further
             if(dir.fix1->file_len > 0) // to start of file data
                    datapos += sizeof(long)+2; // EDAT_SIG + WORD(?)
             else if((cur_vtbl.ver & WIN_MASK) == WIN95) // assume subdir & win95
                    datapos +=18; // some other EDAT_SIG stuff in here!
             // helps but not enough

             // this skips root record which has no name in WinME
             // Win95 style do have a name             
             if(dir.fix1->nm_len > 0 && dir.nm1 != NULL)
             {
                 /* convert first, LFN style, unicode name
                    to a string, made NM_LEN 255 so should fit but validate below
                 */
                 for(i=0,nm_len=0;i<dir.fix1->nm_len;i++)
                 {
                     ch = *(dir.nm1+i++); // unicode, skip alternate chars
                     if(nm_len < NM_LEN)
                        name[nm_len++] = ch;// turncates name to NM_LEN
                 }
                 name[nm_len] = 0; // make it a string

                 if(!(flag & TREE)) // display raw catalog
                 {
                    printf("\n\n%-18.18s %c  %8ld   ",name,dch,dir.fix1->file_len);
                    disp_datetime(dir.fix1->m_datetime);
                    printf("  attrib: %2x",dir.fix1->attrib);
#ifdef VERBOSE
                    // display more of data on 2nd line
                    putchar('\n');
                    disp_datetime(dir.fix1->c_datetime);
                    putchar(' ');
                    disp_datetime(dir.fix1->a_datetime);
                    lptr = (long *)(buf+2);
                    printf("  @ 2 = 0x%4lx",*lptr); // something, no idea....

// mostly zeros, and they same, unknwn3[20] seems to be attributes
                    printf("\nunknww1 %x unknww2 %x ",dir.fix1->unknww1,dir.fix1->unknww2);
                    printf("\n  unknwb1[] = ");
                    for(i=0;i < 20;i++) 
                    {
                      printf(" %02x",dir.fix1->unknwb1[i]);
                    }
                    printf("\n  unknwb2[] = ");
                    for(i=0;i < 4;i++) 
                      printf(" %02x",dir.fix1->unknwb2[i]);
#endif
                 }
                 else  // TREE mode is active
                 {
#ifdef MSDOS        // need to use 8.3 MSDOS names  LFN is NOT supported
                    if(dir.nm2 != 0 && // there is an MSDOS 8.3 style name
                       dir.fix2 != NULL &&  dir.fix2->nm_len > 0)
                    {
                       for(i=0,nm_len=0;i<dir.fix2->nm_len && nm_len< NM_LEN;i++)
                          name[nm_len++] = *(dir.nm2+i++); // unicode, skip alternate chars
                       name[nm_len] = 0; // make it a string
                    }

#endif
                    // dynamically build tree of file/subdir nodes
                    // keep appending till find DIRLAST flag, then adjust
                    if(ccat == NULL)
                    {
                        printf("\nfatal error ccat undefined");
                        break;
                    }
                    else if((tcat = 
                       new_cat_list(ccat,name,datapos,dir.fix1)) == NULL)
                    {
                       printf("\nnew_cat_list() alloc error");
                       break;
                    }
 
                    else 
                    {   // add to list of children
                        if(ccat->child == NULL)
                        {  // create first child
                           ccat->child = tcat;
                           ncat = tcat;
                        }
                        else if(ncat != NULL)
                        {  // append to end of child list
                           ncat->next = tcat;
                           ncat = tcat;
                        }
                        else
                        {
                           printf("\nncat undefined");
                           break;
                        }
                    }
                 }

             }
//           flags are different from QIC113
             if((dir.fix1->flag & DIRLAST) && (flag & TREE))
             /* Before Ralf's work (pre 1/04) thought
                an empty sub dir is a SUBDIR and DIRLAST
                as I had never included an empty subdir in a backup
                but its perfectly valid

                need to adjust CAT_LIST any time see DIRLAST
             */
             {  // printf("\nDIRLAST");
                if(root == NULL) // ignore 1st one for root with WinME
                {
                   root = ccat;  // set root to clear it
                   if((cur_vtbl.ver & WIN_MASK) == WIN95  && // assume win95?
                       ccat->child != NULL)
                   {
                       root = ccat->child; // root has a name
                       root->parent = NULL; // tested by -t for out of bounds
                       free(ccat);
                       ccat = root; // assed backwards way to do it
                   }
                        
                }
                else // normal transition on DIRLAST
                {
                     ncat = ccat; // save current location
                     // try to advance to a child or next node
                     ccat = next_tree_node(ccat);
                     /* if ccat == NULL still, everything below and at this level failed, 
                        try for a parent's next
                        by definition a parent is a subdir, but next may not be....
                        must ignore direct parent and its children
                     */
                     while(ccat == NULL && ncat->parent != NULL)
                     {
                        ncat = ncat->parent; 
                        if(ncat->next != NULL)
                        {
                            if(ncat->next->flag & SUBDIR)
                               ccat = ncat->next; 
                            else
                               ccat = next_tree_node(ncat->next);
                        }
                     }
                     if(ccat != NULL)
                        ncat = ccat->next;
                     else
                        ncat = NULL;
                }
             }
             
             
             if(dir.fix1->flag & DIREND)
             {
                printf("\nend of volume directory");
                printf("\n%d files contain %lx bytes of data, %d directories found",
                        cnt,fdata,dcnt);
                break; // probably VOL END & END this dir
             }
             if(dir.fix1->file_len > 0)
             {
                    datapos += dir.fix1->file_len; // append this files length
                    if((cur_vtbl.ver & WIN_MASK)== WIN95) // assume subdir & win95
                       datapos +=12; // some more EDAT_SIG stuff in here!
             }
         } // while no errors

         // ------- now have a directory tree, use it for display or extract
         if(rcnt)
         {  
            flag = 0; // block other options
            printf("\n extracted %ld files",
               do_redirect(fp,&cur_vtbl,root,paths,rcnt));
         }

         if(flag & TREE)  // show tree 
             disp_tree(root,0,tmode);
         if((flag & EXTRACT)) // look at tree, extract named file
         {
             /* Change logic 1/20/04 must have a named root in extact
                if Win98 format use "ROOT" if WIN98 use actual name
                adds a couple letters must input for -x
                but cleans up -p and @ logic
             */
             ccat = tree_node(root,target,ISFILE);
             if(ccat != NULL)
             {
                 printf("\nfound %s at data section offset %lx length %lx",
                    target,ccat->data_off,ccat->file_len);
                 printf("\nExtracting to current directory");
#ifdef MSDOS
                 printf("\nForcing MSDOS filename (ie 8.3 format)");
#endif
                 if(cur_vtbl.vtbl.comp & 0x80)
                    printf("\nCaution overwrites temporary file %s in process",dcompfnm);
                 do_extract(fp,&cur_vtbl,ccat->name,ccat);

             }
             else
                 printf("\n target %s not found (its case sensitive!)",target);
         }
    }
    fputc('\n',stdout);  // some cleanup for linux
    return(suc); // printf() above and this return makes gcc happy
}                            

