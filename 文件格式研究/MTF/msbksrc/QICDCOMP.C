/* qicdcomp.c  decompression *.qic files, see main() in msqic.c 
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

-----------------------------------------------------------------------------
   History:


   see qic docs: qic122b.pdf
   should add some exit codes etc, got it working quick and dirty
   then for space reasons split out of msqic.c

   10/18/03 the do_decompress is messed up for multiple drives
   under WinME.  add this comment, thought about a patch,
   but a little messy.  Don't need to do this NOW!

   12/13/03 start to change code per above
   do_decompress needs to handle stripping seg_head regions
   from catalog.  In process add passing vtbl_ver to
   do_decompress() instead of just data_off.  Want to be able
   to create a single volume generic archive from one of members
   in Win98 multi volume.

   12/14/03
   change msqic.c so cvtbl->dirbase points to actual base rather
   than offset after cseg_head

   12/22/03  go to FOFFSET and LSEEK_ERR defines in msqic.h
   1/11/08 add Ralf's CYGWIN compiler defines
   2/2/04 add HAS_INT64 logic for Avik, basically just add prt_foff()
          and add calls to it.  Most of FOFFSET logic already in place
          to get me to the 4Gb boundry
   5/03/04 try to make minimal modifications to do_decompress()
          so it will work with a recreated VTBL  See RECREAT tests.
          Also set vtbl.comp = 0, was only clear high order bit

          Looks like can normally determine the length of the data
          region by chasing down the segment chain, but not
          the catalogs which always seem to have cseg_head.seg_sz = 
          SEG_SZ & 0x8000.  So add get_dir_len() routine to chase
          down chain to find length.
   5/31/07 backuped up current version to qicdcomp2.c  Now try to fix
          WIN95 issues per Sven Verhaegen's email of 5/22/07 his sample archive
          us like my single b95c.qic backup. see moeenv~1.qic
          see qicdcmp4.c for modes which depend on the ver info flag
          I create and pass in, but just realized this is DUMB
          both the WIN95 and WINME format advance SEG_SZ bytes
          each time, they just treat shead->seg_sz differently
          Old code: only works for WINME were seg_sz is 0x73F6
              cur_pos += shead.seg_sz; // where we want to be next
          seems to me the WIN95 logic should work for all! try it
              cur_pos += SEG_SZ - sizeof(shead);  // always 0x73F6

          yuk, do_decompress() was incorrectly testing cvtbl->ver
          without WIN_MASK so always assumed WINME format

*/
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h> // define memset
#include <sys/types.h>
#include <sys/stat.h>   /* for open() defines */

#if  defined(MSDOS) || defined(_WIN32) 
# include <memory.h>
# include <io.h>                // for lseek
#else
# ifdef unix
#  include <unistd.h>           // for read/write etc.
//#  define exit     _exit        // ralf likes this..
#  define strnicmp strncasecmp
#  define stricmp  strcasecmp
#  ifndef O_BINARY
#   define O_BINARY 0           // no difference between text and binary mode for LINUX
#  endif
# else
#  error Unknown build environment.
# endif
#endif

#include "msqic.h"

void prt_foff(FOFFSET * off) 
{
#ifndef HAS_INT64
    printf("%lx",*off);
#else
    DWORD *l = (DWORD *)off;
    if(*(l+1) != 0)
       printf("%lx%08lx",*(l+1),*l);
    else
       printf("%lx",*l);  
#endif
}

/*------------DECOMPRESSION see dcomp.c for initial trial ----------*/
#define HBUF_SZ 2048
unsigned char hbuf[HBUF_SZ];  // history buffer

int fin,fout,bits=0,bcnt = 0,hptr=0;
FOFFSET comp_rd=0,comp_wr;

void flush_hbuf()
{
    int i;
    if((i=write(fout,hbuf,hptr)) != hptr)
    {
       printf("\nerror writting output file");
       exit(2);
    }
    hptr = 0;
    comp_wr += i;
}

unsigned char getbit()
{
   unsigned char mask,ret;
   if(bcnt == 0)
   {
       if(read(fin,&bits,1) == 1) // not efficent, but easy
       {
          bcnt = 8;
          comp_rd++;
       }
       else
       {
          printf("\n fatal error reading byte stream");
          flush_hbuf(); // write anything we can....
          exit(1);
       }
    }
    bcnt--;
    if(bcnt < 0)
    {
       printf("\nlogic error getbit() cnt went negative");
       flush_hbuf(); // write anything we can....
       exit(1);
    }
    else if (bcnt == 0)
        mask = 1;
    else
        mask = 1 << bcnt;
    if(mask & bits)
    {
         ret = 1;
         bits &= ~mask; // clear this bit if set
    }
    else
        ret = 0;
    return(ret);
}

unsigned char getbyte()  // for raw char in stream
{
    unsigned char ret = bits;
    int nbit = bcnt; //
    bcnt = 0;
    // this could be done a lot more efficently, but not now
    while(nbit < 8) 
    {
       ret = (ret << 1) + getbit();
       nbit++;
    }
    return(ret);
}

int getsoff()
{
   unsigned short off = 0,bits=7,i;
   if(getbit() == 0)
      bits = 11;
   for(i=0;i<bits;i++)
       off = (off << 1) + getbit();
   return(off);
}
/*  below would save a little space, but not what is shown in *.pdf
   while(1)
   {
      nibble = 0;
      for(i=0;i<2;i++)
         nibble = (nibble < 1) + getbit();
      len = (len << 2) + nibble;
      if(nibble < 3)
          return(len + 2);
   }
*/
int getslen()
{  unsigned int len=0,nibble,i,j;
   // for 2 - 7 check two bits at a time
   for(j = 0; j < 2; j++)
   {
      nibble = 0;
      for(i=0;i<2;i++)
         nibble = (nibble << 1) + getbit();
      if(nibble < 3)
          return(len + nibble + 2);
      else
          len += 3;
   }
   while(1)
   {
      nibble = 0;
      for(i=0;i<4;i++)
         nibble = (nibble << 1) + getbit();
      if(nibble < 15)
          return(len + nibble + 2);
      else
          len += 15;
      j++; // just for fun see how many iterations we make
   }
}

/* use global buffer hbuf[HBUF_SZ] to chase down directoy chain
   to its end.  Return total length in bytes, and restore file
   position to where ever it was at the start

   This got really messy to deal with possibility of bytes
   spanning a cseg_head.  Looks ok now
*/
FOFFSET get_dir_len(int fp, int comp,FOFFSET soff)
{
    FOFFSET ioff,off=soff,len=0,dlen=0; 
    WORD  hsz = sizeof(struct cseg_head);
    struct cseg_head *cseg = (struct cseg_head *) hbuf;
    int brd=0,rd=0,scnt=0,suc = 0;
    BYTE *prd = (BYTE *)&rd; // byte pointer to entry length bytes
    // save current file position,ioff, doesn't not have to be start, soff
    if((ioff = lseek(fp,0L,SEEK_CUR)) == LSEEK_ERR ||  // get current pos
        lseek(fp,off,SEEK_SET) == LSEEK_ERR)        // go to start pos
        suc++;
    while(suc == 0)
    {
        if(comp && len==0) // must skip cseg_headers
        {
           if(read(fp,hbuf,hsz) != hsz)
               suc++;
           else 
           {
               scnt++; // count them for fun
               if((len = cseg->seg_sz & ~RAW_SEG) == 0)
               {
                   printf("\nWarning 0 length segment in catalog");
                   break; // don't think this happens, would be end of chain
               } 
               if(!(cseg->seg_sz & RAW_SEG))
               {
                   printf("\ncompressed segment found in catalog");
                   suc++;
               }               
           }        
        }
        if(suc) break;

        if(comp == 0) // simple case, no compression headers
        {
           if((brd=read(fp,&rd,2)) != 2)
              suc++;
           else if(rd == 0)
              break; // end of catalog
           else if(read(fp,hbuf,rd) != rd)
              suc++;
           else
              dlen += brd + rd;
        }
        else // compressed and len != 0
        {
           // get word len byte by byte as may span cseg_head
           while(brd < 2 && len > 0 && suc == 0)
               if(read(fp,prd + brd++,1) != 1)
                  suc++;
               else
                  len--;

           if(brd == 2 && len > 0 && suc == 0)
           {
               // ok now rd value is valid
               if(rd == 0) // we are done
                   break;
               else if(rd <= len) // get the data
               {
                   if(read(fp,hbuf,rd) != rd)
                      suc++;
                   else
                   {
                      len -= brd + rd;
                      dlen += brd +rd;
                      brd = 0; // clear for next read
                   }                     
               }
               else // can only read part of it, spans a cseg_head
               {
                   if(read(fp,hbuf,(int)len) != len)
                      suc++;
                   else
                   {
                      dlen += len;
                      rd   -=len; // did partial read
                      len = 0;    // will force cseg_head read
                   }                   
               }
           }
        }
    }
    if(lseek(fp,ioff,SEEK_CUR) == LSEEK_ERR)
       printf("\nWarning failed to restore initial file position");

    if(suc)
        return(LSEEK_ERR);
    else
        return(dlen);   
    
}
        

/*-----------------

    do {
        if(rd < 2) // read more into buffer
        {
           if(lseek(fp,off,SEEK_SET) == LSEEK_ERR ||
              (rd = read(fp,hbuf,HBUF_SZ)) < 2) 
              suc++;
           else
              w = (WORD *)hbuf;
        }
                 
        while(suc == 0 && rd >= 2)
        {
           if(comp && (off -soff) % SEG_SZ == 0)
           { all fucked uo
           }
           off += *w +2;  // add data length + length word itself
           rd  -= *w +2;
           w   += *w +2;  // advance pointer (may go past end of hbuf!)
           n++; // count directory entries found
        }      
    }   while(suc == 0 && *w != 0);
----------*/


void decomp_seg()  // was being sloppy, no return!
{ 
    int off,len,i,rawndx=-1,limit=29696;  // was 5000 prior to 10/14/03
    char verb=0;
    FOFFSET foff;

    hptr=0;bits=0;bcnt=0;comp_rd=0,comp_wr=0; // init so repeat calls can be made
    //  early termination so don't worry about buffer wraps
    while(hptr < limit)  // or till terminator, pat == 0x180, off == 0
    {
       if(getbit() == 0)  // raw byte
       { 
         if(hptr == HBUF_SZ)
             flush_hbuf();  
         if(rawndx < 0)  // always track this, use verb to control display
            rawndx = hptr; // get first raw byte entered
         // only raw chars go in history buffer
         hbuf[hptr] = getbyte();
         hptr++;
       }
       else // a string
       {
          if(rawndx >= 0)
          {
             if(verb)
             {
                 printf("\nraw chars: ");
                 for(i=rawndx;i<hptr;i++)
                 {
                   if(hbuf[i] >= ' ') // is printable, not control char
                        putchar(hbuf[i]);
                   printf(" 0x%02x, ",hbuf[i]);
                 }
             }
             rawndx = -1; // end raw data block (1 or more chars)
          }
          off = getsoff();
          if(off == 0)
          {
              foff = (FOFFSET)lseek(fin,0L,SEEK_CUR);
              printf("\nfound compression terminator in byte stream at 0x");
              prt_foff(&foff);
              break; // this is the terminator
          }

          len = getslen(); 
          if(verb)
              printf("\nstring: len = %d offset = %d",len,off);
//        note the unmodified offset is always displayed
#ifdef CARRAY
// the base 0 or 1 for array off handled below
// if don't comment out decrement example is better, qic dcomp worse
          off--; // convert to C array offset where 0 is valid
#endif
          // copy from history buffer to output buffer
// off in *.qic is displacement from current buffer position
// error check it
          if((i = hptr-off) < 0)
          {
              if(verb)
                 printf(
"\nwarning offset %d from current pos %d out of range, wrap it around",
                   off,hptr);
              i += HBUF_SZ; // allow it to continue
          }
          else if (i >= HBUF_SZ)
          {
              if(verb)
                 printf(
"\nwarning offset %d subtracted from current pos %d out of range, use modulo",
                      off,hptr);
              i = i % HBUF_SZ; // wrap it
          }
          while(len-- > 0)
          {
              if(hptr == HBUF_SZ)
                 flush_hbuf();  
              hbuf[hptr++] = hbuf[i++]; // copy in string
              i = i % HBUF_SZ;
          }
       }
           
    }
    if(hptr)
          flush_hbuf();
    printf("\nread 0x");
    prt_foff(&comp_rd);
    printf(" input bytes, generated 0x");
    prt_foff(&comp_wr);
    printf(" output bytes");
}

/* copy from len bytes from fin to fout
   file points must be positioned before call
   side effect comp_wr set to # bytes written
   uses global hbuf[] as transfer area, clobbering any data in it
*/

// similar to below but write len NUL bytes to fout
int pad_seg(FOFFSET len)
{
    int i,sz,suc = 0;
    memset(hbuf,0,HBUF_SZ);  // clear buffer   
    comp_wr = 0;
    while(suc == 0 && len > 0)
    {
       if(len > HBUF_SZ)
           sz = HBUF_SZ;
       else
           sz = len;
       if(sz > 0 && (i = write(fout,hbuf,sz)) != sz)
           suc = -2;
       else
           len -= sz;
    }
    return(suc);
}

int copy_region(long len)
{
    int i,sz,suc = 0;
   
    comp_wr = 0;
    while(suc == 0 && comp_wr < len)
    {
       if(len - comp_wr > HBUF_SZ)
           sz = HBUF_SZ;
       else
           sz = len - comp_wr;
       if(sz > 0 && ((i = read(fin,hbuf,sz)) != sz ||
                          write(fout,hbuf,sz) != sz))
           suc = -2;
       else
           comp_wr += sz;
    }
    return(suc);
}

/* add top level logic
   assumes fo is a the beginning of a new empty file.
   seeks back and rereads VTBL on fi

   Needs work for multiple drives in WinME, see get_vtbl()

   Originally hoped could convert from one format to another,
   but its more than the header that is different, so give that up.
   VER 1.02 copied database bytes from beginning of original file to 
      new file.  Thus allowing recreation of Win95 or WinME style.

   Now check cvtbl->ver and ->cnt to determin if MDID required
  
   12/13/03 change from int database
   to struct vtbl_ver *cvtbl
   keep a local copy of modified header and write at end

   2/5/04 ah ha just realized what vtbl.start and .end
   fields are really for.  Normally change when decompress a
   volume, attempt to add this logic

   Oh dear, in process find never dealt with multi-volume
   WINME stuff, ie never did seek(fin, to cvtbl->database

   4/28/04 add logic for cvtbl->OSver == RECREAT
   to do this easily remove test below on cum_wr in read loops
   This may allow archive larger than 4GB, and solve the problem
   that one does NOT know dataSz for RECREAT case
         cum_wr >= (long)vtbl.dataSz[0])   // safety net...

   Remove ability to use -sc## to set location of catalog for RECREAT
   assume it immediately follows data region.  This makes it 64 bit compatible.

   6/12/07 added some breaks in main loop on error and about a week
   ago logic to change ad

*/
int do_decompress(int fi,int fo,struct vtbl_ver *cvtbl)
{
    int i,suc = 0,sz,nshead=1; // always reads 1 seg_head past last segment
    FOFFSET cum_rd = 0,cum_wr=0,cur_pos,pad;
    struct qic_vtbl vtbl;
    struct cseg_head shead;
//  minimize parameter passing by making fin and fout locally global
    fin = fi; fout = fo;
    sz = sizeof(struct qic_vtbl);

    if(!(cvtbl->vtbl.comp & 0x80))
    {
        printf("\nimage is not compressed, nothing to do!");
        suc = 1; // do nothing
    }
   
    else if((cvtbl->ver & WIN_MASK) == WINME) // obtain and write MDID region
    {
        if(cvtbl->ver & RECREAT) // create empty block with tag
        {
            memset(hbuf,0,sz);
            strncpy(hbuf,"MDID",4);
            // required so parsed correctly after decompression
        }
        else // get copy of existing 
        {
            cur_pos = 128L*cvtbl->cnt;
            if (lseek(fin,cur_pos,SEEK_SET) != cur_pos ||
                (i = read(fin,hbuf,sz)) != sz)
               suc = 3;
        }

        if( suc == 0 && lseek(fout,(FOFFSET)128,SEEK_SET) != 128 ||             
            (i = write(fout,hbuf,sz)) != sz)
            suc = 3;
        else
            sz *= 2; // bytes in header region VTBL + MDID
        if(suc != 0)
            printf("\nerror creating WinME MDID");
    }
    

    if((FOFFSET)lseek(fin,cvtbl->database,SEEK_SET) !=cvtbl->database)
    {
        printf("\nerror positioning input file to data region");
        suc = 2;
    }
    else if((FOFFSET)lseek(fout,(FOFFSET)sz,SEEK_SET) != (FOFFSET)sz)
    {
        printf("\nerror positioning output file after vtbl");
        suc = 2;
    }


    if(suc == 0) 
    {
        // get copy of vtbl in local buffer
        memcpy((void *)&vtbl,(void*)&cvtbl->vtbl,sizeof(struct qic_vtbl)); // save for mods
       /* Compressed dir starts with a cseg_head, must pad to next seg boundry
           as remove cseg_head from dir, so dir now starts earlier
           Note in Win98&ME dirSz = # segments * SEG_SZ
                in Win95    dirSz = space used in segment
                in RECREAT  don't know them obtain from seg_heads, force WIN95 format
           12/13/03 couple issues here
           a) may be more than one segment I ignored that and
              the additional seg_heads that must be removed previously
           b) what if its a multi drive backup from WinME/98??
              Answer: only write the currently selected volume
        */
        vtbl.comp = 0; // clear compressed byte
        if((cvtbl->ver & WIN_MASK) == WINME)
           vtbl.start = 3; // I only write single volumes starting after VTBL
        /* was hopping to make ME to 95 conversion, but takes more work
           there are extra bytes in data stream....
        */
    }
    while(suc == 0)
    {
        if((i=read(fin,&shead,sizeof(struct cseg_head))) !=
            sizeof(struct cseg_head))
        {
            suc = -2;
            break; // 6/12/07 can't do anything more exit loop!
        }
        else if(shead.seg_sz == 0 || // end of data
             (!(cvtbl->ver & RECREAT) && cum_wr >= (long)vtbl.dataSz[0])) // saftety net 
        {
            printf("\nDecompressed all data segments");
            break;
        }
        // get current position, may need to seek ahead to next block
        else if((cur_pos = lseek(fin,0L,SEEK_CUR)) == LSEEK_ERR)
        {
            suc = -1;
            break; // 6/12/07 shouldn't do anything more exit loop!
        }
        else if(shead.seg_sz & RAW_SEG) // don't decompress this one!
        {
            printf("\n\nRaw copy of uncompressed segment starting at 0x");
            prt_foff(&cur_pos);
            // do a raw copy of the segment
            suc = copy_region((long)(shead.seg_sz & ~RAW_SEG));
            comp_rd = comp_wr; //setup for totals at end of loop
        }
        else // decompress the segment
        {
            printf("\n\nDecompress segment starting at 0x");
            prt_foff(&cur_pos);
            decomp_seg();  // currently has no return!
            // input file position is a little random, do a seek
            // but force advance to SEG_SZ between headers
            cur_pos += SEG_SZ - sizeof(shead);  // always 0x73F6
            if(lseek(fin,cur_pos,SEEK_SET) != cur_pos)
                suc = -1;
        }
         nshead++; // read another seg_head, required for RECREAT
         cum_rd += comp_rd;
         cum_wr += comp_wr;
         printf("\ncumulative decompressed bytes written 0x");
         prt_foff(&cum_wr);       

    }
    /* must still write table of contents to complete process
       12/13/03 It turns out although a compressed files table
       of contents is not compressed, it does have seg_head regions
       which must be stripped  (stupid, but true)
    */
    if(suc == 0)
    {
       if((cvtbl->ver & WIN_MASK) == WINME)
       {
       // update end field, must be done before pad_seg() overwrites cum_wr
          vtbl.end = 3; // base 
          vtbl.end += cum_wr/SEG_SZ; // point to 1st directory segment
          if(cum_wr%SEG_SZ)
              vtbl.end++;  // round up
       }
       if(cvtbl->ver & RECREAT)
       {
          vtbl.dataSz[0] = cum_wr;
          // assume no overflow into upper 32 bits, vtbl.dataSz[1] 
       }
       else if(cum_wr != vtbl.dataSz[0]) // ignore high order long
          printf("\nWarning: Decompressed Data bytes != dataSz");
       printf("\n append a copy of the original directory");
       // fudge it, use dataSz as abs position by include of VTBL sizes
       // compressed files dataSz is actually the decompressed size!
       // dir must start on a Segment boundry
       // for WinME, two 128 byte headers before data
       // for Win95, only one
       pad = vtbl.dataSz[0] % SEG_SZ;
       if(pad)
         pad = SEG_SZ - pad;
       if((cur_pos = lseek(fout,0L,SEEK_CUR)) > vtbl.dataSz[0]+sz) // have written too far
           suc = -1; // separate these for debugging so I can see who fails
       else if(pad && pad_seg(pad) != 0) // pad output data region to next segment
           suc = -2;
       else // add RECREAT test to set dirbase, assume next segment
       {
          if(cvtbl->ver & RECREAT)
          {
             if(cvtbl->dirbase == 0) 
             {
                 // if it was set from command line it won't be zero
                 cvtbl->dirbase = cum_rd + nshead * sizeof(struct cseg_head);
                 pad = cvtbl->dirbase % SEG_SZ;
                 if(pad)
                     cvtbl->dirbase += SEG_SZ - pad;
                 cvtbl->dirbase += cvtbl->database; // add in start offset
             }
             if((pad = get_dir_len(fin,1,cvtbl->dirbase)) == LSEEK_ERR)
             {
                 printf("\nError determining catalog length");
                 suc = -4;
             }
             else
                 vtbl.dirSz = pad;
          }
          if(suc == 0 && (cur_pos = lseek(fin,cvtbl->dirbase,SEEK_SET)) !=cvtbl->dirbase) 
             suc = -3; // failed to move to dir cseg_head        
       }

       if(suc == 0)
       {
           /* may want validation above, below assumes we are at seg_head
              strip cseg_head regions
           */ 
           nshead = 1;
           cum_wr = 0;
           do {
               if(read(fin,&shead,sizeof(struct cseg_head)) != sizeof(struct cseg_head))
                  suc = 5;
               else if((shead.seg_sz & RAW_SEG) == 0)
               {
                  printf("\ncompressed catalog detected???");
                  suc = 6;
               }
               else if((pad = (shead.seg_sz & ~ RAW_SEG)) == 0)
                  break; // done with directory [oops doesn't happen in single volume reach EOF]
               else if(copy_region((long)pad) != 0)
                  suc = 5;
               else
                  cum_wr += comp_wr;
               if(cum_wr >= vtbl.dirSz) break;
           } while(suc == 0); 
           // if(cvtbl->ver & RECREAT) don't know dirSz, set from cum_wr
           if(suc == 5 && (cum_wr == vtbl.dirSz || cvtbl->ver == RECREAT))
              suc = 0;  // read it all, probably a single volume archive
           pad = cum_wr % SEG_SZ;
           if(suc == 0 && pad)
           {
               pad = SEG_SZ - pad; 
               suc = pad_seg(pad); // go to next seg boundry
           }
           if((cvtbl->ver & WIN_MASK) == WINME)
               vtbl.dirSz = cum_wr +pad;
           else
               vtbl.dirSz = cum_wr;  // WIN95 std
       }
           
       sz = sizeof(struct qic_vtbl);

       if(suc != 0)
           printf("\nerror %d during copy",suc);
       else if(lseek(fout,0L,SEEK_SET) != 0L ||
            (i = write(fout,&vtbl,sz)) != sz)
       {
            printf("\nerror writing final VTBL header");
            suc = -2;
       }

    }
    close(fout);
    return(suc);
}

