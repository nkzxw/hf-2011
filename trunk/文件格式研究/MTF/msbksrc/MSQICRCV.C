/* msqicrcv.c  - support routines for main() in msqic.c
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

11/31/03 
split out of msqic V1.01 11/31/03
add write_vtbl() stub
copy current version to msqicrc1.c
   it had path error and seems to be in an infinit loop.
   exceeds path length, breaks out of search and then
   tests for EDAT_SIG beyond end of buffer.  This fials
   off wasn't updated so calls next_data_sig with dataoff
   that failed last time....

12/05/03 seems ok under linux also, but reguires a strupr.c
    modules, must look equivantlent unix funcion and define.
12/16/03 wrote and tested write_vtbl(), seems ok
12/22/03 add some code changes, ie FOFFSET and LSEEK_ERR
   so new defines in msqic.h for _4GB can work  affected next_data_sig()
   will probably screw up some of Ralf's warning issues again....

   also append tree routines and do_extract() as ran out of space
   for ed in msqic.c

1/8/04  was out of sync with Ralf's stuff, see sub dir \ralf
   Apparently his didn't crash with '-t' but mine did.
   see notes in msqicsrc.lst

   apparently disp_tree() blew up if nesting was too deep
   checked out by reducing nesting level. see new IND_LEV define
   Yes this seems to produce problem, have corrected so appending
   last NUL doesn't overwrite stack.  

   Also added EMPTYDIR logic based on Ralf's info.
   seems to work with my test file: \tempty2.qic

   very odd, do_extract() does fgets() to obtain file name
   but doesn't wipe /n so open fails.  How did it every work?
   Oh I bet I started with gets() converted to linux and never rechecked it

1/11/04 diddle do_extract() changing arguments so pass in
   ccat pointer with file name.  Also change tree routines to used
   MSDOS name if MSDOS defined and it exists (ie there is a long name)
   add ralf's CYGWIN compiler defines
   add mode to disp_tree so can change mode at run time
   add get_paths(), tree_copy(), and do_redirect()
   required new parameter in call to tree_node()
1/15/04 do_extract was decompression one too many segments
   see off by one error on incrementation
   testing with big note by change
   found some more FOFFSET issues and an error in  next_data_sig() 
   major error in logic to estimate file length in do_recover()
   how did it ever work.
1/17/04 add check for ".\" as redirect path in get_paths()
   tweak do_redirect() to ignore empty redirect path which
   is current directory and must exits.
   also required changing where tree_path() appends DELIM
1/22/04 start adding setting timestamp and attribute on file extraction
    and '+' option in do_redirect to create directories.
    remove strupr() again, only required if MSDOS when it exists in stdlib
1/24/04 in do_redirect() add ccat != NULL before copy_tree(..ccat->child
    in get_paths() add incrementation of i while looking for redirect path
2/08/04 error in do_recover() needed to save attrib and datetime
     before next_data_sig() call, was restoring incorrectly!
3/19/04 tweak get_paths() to match nttree.c, think it didn't handle
     termination of quoated destination correctly
4/28/04 add create_vtbl() attempt to create a vtbl if the archives is
     unreadable.
5/03/04 oops need to set cvtbl->ver not cvtbl->vtbl.OStype!
*/
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>  // for toupper()
#include <string.h> // define memset
#include <sys/types.h>
#include <sys/stat.h>   /* for open() defines */

#if  defined(MSDOS) || defined(_WIN32) 
# include <memory.h>
# include <io.h>                // for lseek
#include <sys/utime.h>  // for utime()
#else
# ifdef unix
#  include <unistd.h>           // for read/write etc.
#  include <utime.h>
#  define strnicmp strncasecmp
#  define stricmp  strcasecmp
// note I have a custom strupr() below
# else
#  error Unknown build environment.
# endif
#endif

#include "msqic.h"

extern int bques();


/* trash global buffer looking for start next DAT_SIG
   note 11/30/03 was working with just lptr and code below for WinME 
   if( *lptr == DAT_SIG && *(lptr+1) == UDEF_SIG && *(lptr+2) == UDEF_SIG)
   for win95 the WORD after DAT_SIG is defined, so this test failed
   new one should be generic
   1/15/04 oops! revisting, when I made change above I left
   minsz = 3*sizeof(long)
   must add sizeof(WORD) I've added
   
   This is looing for following hex pattern of bytes where ?? is don't care
   {CC,33,CC,33,??,??,FF,FF,FF,FF,FF,FF,FF,FF}
*/
FOFFSET next_data_sig(int fp,FOFFSET off,BYTE *buf,WORD bsz)
{
    int i,rd,suc=0,minsz = 3*sizeof(DWORD) + sizeof(WORD);
    DWORD *lptr,*lptru;    
    while(suc == 0)
    {
        // read minsz more than need each pass, 
        // then inc off by rd - minsz so end up seeking back minsz
        if((FOFFSET)lseek(fp,off,SEEK_SET) != off || 
           (rd = read(fp,buf,bsz)) < minsz)
             suc = -1;
        else   
        {
           
           i=0;
           do {  
              lptr = (DWORD *)(buf+i);
              // skip WORD which may be defined in Win95 format
              lptru = (DWORD *)(buf+sizeof(DWORD)+sizeof(WORD) +i);
              if( *lptr == DAT_SIG)
              {
                 if( *(lptru) == UDEF_SIG && *(lptru+1) == UDEF_SIG)
                    suc = 1;
              }
              if(suc == 0)
                 i++;
           } while(suc == 0 && i<rd-minsz);
           off += i;
        }
    }
    if(suc > 0)
       return(off);
    else
       return(LSEEK_ERR);
}

int write_file(char *nm,int fp,FOFFSET start,FOFFSET end,BYTE *buf,WORD bsz)
{
   int fo=-2,wr,rd,ret=0;
   FOFFSET l;
   if ( (l = (FOFFSET)lseek(fp,start,SEEK_SET)) != start ||
        (fo=open(nm,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,
                        S_IREAD|S_IWRITE)) == EOF)
       ret = -1;
   while(ret == 0 && start < end)
   {
       if(bsz + start < end)
         rd = bsz;
       else
         rd = end - start;
       if((wr = read(fp,buf,rd)) != rd || (rd = write(fo,buf,wr)) != wr)
         ret = -2;
       start += rd;
   }
   if(fo > 0)
       close(fo);
   return(ret);
}

   

int is_match(char *str,char *filter)
{
    char ret=1;
#ifdef MSDOS
    strupr(str); // DOS name compare, case insensitive
    strupr(filter);
#endif
    while(*str != 0 && ret == 1)
    {
        if(*str == '.' && *filter == '*')
            filter++;
        if(*filter == '*' || *filter == '?') // match all
            str++;
        else if (*filter++ != *str++)
            ret = 0;
        if(*filter == '?')
            filter++;        

    }
    return(ret);    
}


/* set_fattrib() to set both the datetime, and file attribute after written
   called from do_recover() and do_extract()
*/
int set_fattrib(char *name,DWORD datetime,BYTE attrib)
{
    struct utimbuf times;
    int i,suc=0;
    times.actime = times.modtime = datetime;
    if(utime(name,&times) != 0) // set time stamp
         suc = 1;

    i = S_IFREG | S_IREAD; 
    if((attrib & 1) == 0) // its writable
        i |= S_IWRITE;
        /* via generic chmod, I fail to see how to do DOS
               hidden   0x20  (under linux can insert '.' at start name)
               system   0x40
               archive  0x20
               executable in DOS is done by file name extension
        */
    if(chmod(name,i) != 0)
         suc |= 2;
    if(suc)
         printf("\nerror setting file: ");
    if(suc & 1)
         printf("timestamp ");
    if(suc & 2)
          printf("attributes");
    return(suc);
}

/* try to parse data region 
   use the DOS 8.3 file names

   Tricky as don't know file length! not in valid in fixed region

   think will need extra for Win95?

   bitmap defines below for cntrl byte set interactively

move to msqic.h
#define QUERY 1
// EXTRACT 2 defined above
#define W95 4  // else its W98 version of archive


11/31/03
   1st mode, change to incrementally read buffer via calls to 
   next_dir()

extra=6 defined for W98 as {DWORD,WORD} the EDAT_SIG and WORD following
   it in Win 98 files, It turns out to be the same in Win95 files,
   but is 3 * large, ie 18 bytes = 3 pairs for subdir entries.
   since I ignore them, its always 6 below.

However   
extrae=0 for Win98, but for W95 there are 12 extra words after the
   file before the next DAT_SIG, ie if don't do the format properly
   on gets files that are 12 bytes too long.
12/14/14 initialize dir.pseg = NULL to show we are parsing
   a data region, not a catalog
1/15/04 weird, after searching for next_sig after start file did:
   if(next_sig != LSEEK_ERR && (next_sig = lseek(fp,0L,SEEK_END)) > dataoff)
                 printf("\nWarning searched to EOF with no DAT_SIG");
   what was I thinking?
   add dataend and change both to DWORD as set from
   cur_vtbl.database and cur_vtbl.dirbase in main
*/



WORD do_recover(int fp,DWORD dataoff,DWORD dataend,BYTE *buf,WORD bsz,char *filter)
{
   BYTE attrib;
   DWORD datetime;
   int rd,i,off,extra=6,extrae=0;
   WORD fcnt=0,xfcnt=0,tcnt=0;
   FOFFSET next_sig;
   char name[15],*ch,cntrl=W98,match = 1,warn=0;
   struct dir_blk dir; // storage space for dir pointers
   printf("\nDATA recovery options:");
   printf("\n  Default is Win98 format, use Win95 instead (Y/N)? ");
   if(bques(0) == 1)
   {
      cntrl = W95;
      // adjust extra for additional data after 1st EDAT_SIG
      extrae=12; // two pairs of {DWORD,WORD} before next DAT_SIG
   }
   printf("\n  Default displays files, extract them (Y/N)? ");
   if(bques(0) == 1)
   {
      cntrl |= EXTRACT;
      printf("\n  Default, interactive query for each file, extract all (Y/N)? ");
      if(bques(0) != 1)
         cntrl |= QUERY;
      if(filter != NULL && strlen(filter) > 0)
      {
         printf("Match extracted files to: %s (Y/N)? ",filter); 
         if(bques(0) != 1)
              filter = NULL;
      }

   }


   dir.pseg = NULL; // signal not a compressed catalog region
   while((dataoff = next_data_sig(fp,dataoff,buf,bsz)) != LSEEK_ERR &&
          dataoff < dataend &&
          (FOFFSET)lseek(fp,dataoff+sizeof(DWORD),SEEK_SET) == dataoff+sizeof(DWORD))
   {   // now pointing to start of a File spec, read it
       dataoff += sizeof(DWORD);
       off = 0;
       tcnt++; // increment total count think we found something
       if((rd = next_dir(fp,&dir,buf,bsz,cntrl)) > 0)
       {
           off += rd; // bytes read in record
           // ignore if subdir 0x10 or volid 0x8
           if(dir.fix1->attrib & 0x18)
           {
              if(dir.fix1->attrib & 0x10 && warn == 0 && fcnt == 0) 
              /* 1st hit on subdir verify format
                 W95 subdir has two extra pairs of {EDAT_SIG,WORD}
                 but the file entries don't
              */
              {
                   if(cntrl & W98  &&
                      *((DWORD *)(buf+off+dir.fix1->path_len+6)) == EDAT_SIG)
                      printf("\nWarning, looks like Win95 format, files may be 12 bytes too long");
                   else if(cntrl & W95  &&
                      *((DWORD *)(buf+off+dir.fix1->path_len+6)) != EDAT_SIG)
                      printf("\nWarning, looks like Win98 format, files may be 12 bytes too short");
                   warn++; //only give message once
               }
               dataoff += off;
               continue;
           }
       }
       else
       {
           printf("\nfatal error reading dir spec @%lx skip to next",
                    dataoff);
           dataoff += off; // search for next
           continue;
       }

       // next_dir() does NOT read path info, not valid in a catalog
       rd = dir.fix1->path_len + extra;
       if(rd < 1 || rd > bsz -off || dir.nm2 == NULL ||
          (i = read(fp,buf+off,rd)) != rd || 
          *((DWORD *)(buf+off+dir.fix1->path_len)) != EDAT_SIG)
       {
          printf("\nerror verifing or reading to start data file @%lx skip to next",
                 dataoff);
          continue;
       }
       
       fcnt++; // found a valid file            
       dataoff += off + rd; // skip extra words to start file
       ch = dir.nm2; // start MSDOS name
       for(i=0;i<dir.fix2->nm_len/2 && i < 14;i++)
       {
           name[i] = *(ch++);
           ch++; // skip 2nd unicode
       }
       name[i] = 0;
       if(filter != NULL)
            match = is_match(name,filter);
       if(match)
       {   
            printf("\n@%lx  Path: ",dataoff);
            for(i=0;i<dir.fix1->path_len;i++)
            {
                  if(*ch < ' ')
                      fputc(DELIM,stdout); // 0 or 0xA ?
                  else
                      fputc(*ch,stdout); 
                  ch++;ch++;i++; // skip 2nd unicode byte
            }
            // must save these, io below overwrites them as use global buf
            attrib = dir.fix1->attrib;
            datetime = dir.fix1->m_datetime;
            // overwrite buffer to find file length, destorys dir.* ptrs
            next_sig = next_data_sig(fp,dataoff,buf,bsz);
            if(next_sig == LSEEK_ERR || next_sig > dataend)
            {
                if(next_sig == LSEEK_ERR)
                   printf("\nSearched to EOF with no next DAT_SIG, ");
                else
                   printf("\nNext DAT_SIG after end of data, ");
                printf("use end data region %lx",dataend);
                next_sig = dataend;
            }
            if(next_sig < dataoff)
                 printf("\ncan't estimate length for %s",name);
            else
            {  
               if(match && cntrl & EXTRACT)
                       i = 1;
                    else 
                       i = 0;
              
               if(match)
               {
                   if(cntrl & QUERY)
                   {
                        printf("\nsave as %14s est length %lu (Y/N) ?",
                            name,(DWORD)next_sig-dataoff);
                        i = bques(0); // set to 0 to skip save
                    }
                    else 
                        printf("\n%14s est length %lu",name,(DWORD)next_sig-dataoff);
               }
               if(i)
               {
                   if((i=write_file(name,fp,dataoff,next_sig,buf,bsz)) != 0)
                      printf("  - error %d saving file\n",i);
                   else
                   {
                      set_fattrib(name,datetime,attrib);
                      xfcnt++;
                   }
               }
               dataoff = next_sig; // always advance
             }
       }
   }
   printf("\n%u potential files, %u parsed file names",tcnt,fcnt);
   if(xfcnt > 0)
     printf(", %u files extracted\n",xfcnt);
   return(fcnt);
}

#define VTBL_SZ 128


char *key_reg[] = {"VTBL","MDID",NULL};
char *reg_opts[] = {"null","read",NULL};
char *key_words[2][7] =
  {
   {"desc","flag","dirsz","datasz","label","comp",NULL},
   {"MediumID","VR","CS","FM","UL","DT",NULL},
  };

#define MDID_SZ 6
#define MDID_TERM ((char) 0xb0)
char *mdid_str[MDID_SZ];


int has_key_word(char *targ,char *str[],char **args)
{
    int len=0,klen,i = 0;
    while(*targ == ' ')
         targ++;
    while(*(targ+len) != ' ' && *(targ+len) != 0)
         len++;

    if(args != NULL)
    {
       *args = targ+len;  // return start of args
       while(**args == ' ')
           (*args)++;
    }
    while(len > 0 && str[i] != NULL)
    {
        klen = strlen(str[i]);
        if(len >= klen && strnicmp(targ,str[i],klen) == 0)
              return(i);
        i++;
    }
    return(-1);
}


int get_mdid(char *buf)
{
     int i,key,len,klen,suc = 0;
     char *str,*arg;
     for(i=0;i<MDID_SZ;i++)
        mdid_str[i] = NULL;
     i = 4; 
     while(suc == 0 && i < VTBL_SZ)
     {
         len = klen = 0;
         // pass NULL args these strings have no white space
         if((key = has_key_word(buf+i,key_words[1],NULL)) >= 0 &&
            key < MDID_SZ)
         {
            klen = strlen(key_words[1][key]);
            arg = buf+i+klen;
            while(*(arg+len) != MDID_TERM && i+klen+len < VTBL_SZ)
                 len++;
 
            if(len == 0)
                str = NULL;
            else
            {
               if((str = malloc(len+1)) == NULL)
                   suc++;
               else
               {
                   strncpy(str,buf+i+klen,len);
                   *(str+len) = 0; // make it a string
               }
            }
            mdid_str[key] = str;
        
         }
         i+= len+klen;
         while(*(buf+i) != MDID_TERM && i < VTBL_SZ)
                 i++;
         i++;
      }
      return(suc);
}


void put_mdid(char *buf)
{
   int i,j=4,len,klen;
   memset(buf+j,0,VTBL_SZ-j);

   for(i=0;i<MDID_SZ;i++)
   {
        if(mdid_str[i] != NULL)
        {
           len = strlen(mdid_str[i]);
           klen = strlen(key_words[1][i]);
           if(j+len+klen+1 > VTBL_SZ)
           {
              printf("\nskipping MDID: %s",mdid_str[i]);
              continue;
           }
           strcpy(buf+j,key_words[1][i]);
           j+=klen;
           strcpy(buf+j,mdid_str[i]);
           j+=len;
           *(buf+j) = MDID_TERM;
           j++;
        }
   }

}

#define LN_LEN 100
/* start write data to VTBL
   Each key_reg[] word advances loc by 128 bytes
      if any changes done, they are written
   ie following would write new UL to MDID at offset 128
-------
   VTBL
   MDID  read
   UL a new title
-------
*/
int write_vtbl(char *qicnm,char *datnm)
{
    int fp=EOF,suc=-1,vcnt=-1,hits=0,reg=-1,key,v,tst;
    FILE *fd = NULL;
    FOFFSET off;
    DWORD lv,*lptr;
    char *args,*lret,ln[LN_LEN+1],buf[VTBL_SZ],*str;
    struct qic_vtbl *vtbl = (struct qic_vtbl *) buf;
    if ((fp = open(qicnm,O_BINARY|O_RDWR)) == EOF)
       printf("\nFailed to open archive %s",qicnm);
    else if ((fd = fopen(datnm,"r")) == NULL)
       printf("\nFailed to open VTBL data file %s",datnm);
    else
    {
      do
      {
         lret = fgets(ln,LN_LEN,fd);
         v = strlen(ln)-1;
         if(v >= 0 && *(ln+v) == '\n')
              *(ln+v) = 0; // remove CR
         
         if(lret == NULL || (key=has_key_word(ln,key_reg,&args)) >= 0)
         {  // terminate current region, and optionally start a new one
            if(hits && vcnt >= 0)
            {
              off = vcnt * VTBL_SZ;
              if(reg == 1) // MDID
                put_mdid(buf); // fill buffer with new strings
              if((FOFFSET)lseek(fp,off,SEEK_SET) != off ||
                 write(fp,buf,VTBL_SZ) != VTBL_SZ)
              {
                 printf("\nerror writting region at %lx",off);
                 break;
              }
            }
            if(lret == NULL)
                break; // we are done
            reg = key;
            hits = 0;
            vcnt++;
            if((key = has_key_word(args,reg_opts,NULL)) >= 0)
            {
                if(key == 0) // null it out
                {
                   hits = 1; // force overwrite on close
                   memset(buf,0,VTBL_SZ);
                }
                else // read it in
                {
                    off = vcnt * VTBL_SZ;
                    if((FOFFSET)lseek(fp,off,SEEK_SET) != off ||
                       read(fp,buf,VTBL_SZ) != VTBL_SZ)
                    {
                       printf("\nerror reading region at %lx",off);
                       break;
                    }
                    
                }
                if(reg ==1 && get_mdid(buf))
                {
                    printf("\nfailed to parse MDID strings");
                    break;
                }
                // force tag
                lptr = (DWORD*) key_reg[reg];
                *((DWORD *)buf) = *lptr; // set keyword identifier
            }
         }
         else if((key=has_key_word(ln,key_words[reg],&args)) >= 0)
         {

            printf("\nkey: %s  arg %s",key_words[reg][key],args);
            hits++;            
            if(reg == 0) // doing a VTBL
            {
               tst = 1;
               switch(key)
               { //"desc","flag","dirsz","datasz","label","comp"
               // should desc and lable be padded with blanks?
               case 0: // desc
                     strncpy(vtbl->desc,args,44);
                     break;
               case 1: // flag
                     if((tst=sscanf(args,"%x",&v)) == 1)
                        vtbl->flag = v & 0xff;
                     break;
               case 2: // dirsz
                     if((tst=sscanf(args,"%lx",&lv)) == 1)
                        vtbl->dirSz = lv;
                     break;
               case 3: // datasz
                     if((tst=sscanf(args,"%lx",&lv)) == 1)
                        vtbl->dataSz[0] = lv;
                     break;
               case 4: // label
                     strncpy(vtbl->sdrv,args,16);
                     break;
               case 5: // comp
                     if((tst=sscanf(args,"%x",&v)) == 1)
                        vtbl->comp = v & 0xff;
                     break;
               }
               if(tst != 1)
                  printf("\nfailed to parse: %s",ln);
            }
            else if(reg == 1 && key < MDID_SZ)
            {
                hits++;
                tst = strlen(args);
                if(tst == 0)
                   str = NULL;
                else if((str = malloc(tst)) == NULL)
                {
                   printf("\nalloc error for: %s",args);
                   break;
                }
                else
                   strcpy(str,args);
                  
                if(mdid_str[key] != NULL)
                   free(mdid_str[key]);
                mdid_str[key] = str; // new string
                
// MediumID - unique 19 decimal digits for identification
// VR       - version? always 0100
// CS       - ? followed by 4 hex bytes
// FM       - ? always followed by '2'?  format?
// UL       - user label, ascii input string
// DT       - datetime of archive creation as 8 hex bytes
            }
         }
      } while(lret != NULL);
    }
    if(fp != EOF)
         close(fp);
    if(fd != NULL)
         fclose(fd);
    return(suc);
}

int getnstr(char *buf,int len)
{   
    int ret;
    char *res;
    if((res = fgets(buf,len,stdin)) != NULL)
    {
        ret = strlen(buf);
        if(ret > 0 && buf[ret-1] == '\n')
            buf[--ret] = 0; // remove it
    }
    return(ret);
}

int create_vtbl(struct vtbl_ver *cvtbl)
{
    int i,len,ret=0;
    char cdate[15];
    while(ret == 0)
    {
        printf(
"\nVTBL memory image has been cleared, Win95 output format is set");
        printf(
"\nThe offsets for data and catalog set will be obtained from -sd and -sc");
        printf(
"\n  if the -sd option is omitted the catalog is assumed to follow last data segment");
        memset((char *)cvtbl, 0, sizeof(struct vtbl_ver));
        strncpy(cvtbl->vtbl.tag,"VTBL",4);
        cvtbl->ver = RECREAT;
        cvtbl->vtbl.OStype = 7; // Win 95 format, I ignore, but use this as default
        printf(
"\nDefault is WINME format, ie first segment of each volume's data not compressed");
        printf("\nIs this actually WIN95 format - ie no MDID region (Y/N) ");
        if(bques(0) == 1)  // this controls headers written
            cvtbl->ver |= WIN95;
        else
            cvtbl->ver |= WINME;  

        cvtbl->vtbl.rev_major = 1;  // these are my ver # not MicroSoft
        cvtbl->vtbl.rev_minor = 11;
        printf("\nWhat date would you like displayed (format mon/day/year): ");
        i = getnstr(cdate,14);
// never understood this fudge, see msqic.c:disp_vtbl()
// my file dates work why does this require the fudge
        cvtbl->vtbl.date = mk_date(cdate) + VTBL_DATE_FUDGE;        
        cvtbl->cnt = cvtbl->ndx = 1; // make do_decompress do WIN95 style output
        printf("\nForce data compression flag? (Y/N) ");
        if(bques(0) == 1)
        {
            cvtbl->vtbl.comp = 0x81;
        // only update descriptions for compressed file, only time vtbl may be written
            printf("\nDevice Lable (max 15 chars): ");
            i = getnstr(cvtbl->vtbl.sdrv,15);
            printf("\nDescription (max 43 chars): ");
            i = getnstr(cvtbl->vtbl.desc,43);
        }
        printf("\nESC to abort, 'N' repeat above, or 'Y' to use above: ");
        ret = bques(27);
    }
    if(ret == 1)
       ret = 0;
    else
       ret = 1;
    return(ret);
}

/* from msqic.c on 12/22/03 */
// directory tree routines are recursive (ie not efficient, but easy)

#define IND_LEV 25 // was hard coded as 10

void disp_tree(CAT_LIST *ccat,int level,BYTE mode)
{
    BYTE i;
    // make static so they don't take a lot of stack space
    static char indent[IND_LEV*2+1]; // need a +1 for trailing nul
                               // definately was crashing without it
    if(level <= 0)
    {
       i = 0;
       indent[0] = 0;
       if(mode & DIRONLY)
           printf("\nListing of directory tree, no FILES displayed");
    }
    else
    {
       i = (level-1) *2;
       if(i < IND_LEV)
          strcpy(&indent[i],"  ");
    }
    while(ccat != NULL) // remove debug break on: cnt++ < 30
    {   // add offsets in segment 10/8  off a little if compressed
        if(!(mode & DIRONLY) || (ccat->flag & SUBDIR)) // display name
        {
            printf("\n%s%s",indent,ccat->name);
            if(mode & S_SEGS)  // display segment/offset debug info
               printf("  %lx:%lx",
                  ccat->data_off / SEG_SZ, ccat->data_off % SEG_SZ);
        }
        if(ccat->child != NULL)
             disp_tree(ccat->child,level+1,mode);
        ccat = ccat->next;
    }
    indent[i] = 0; // remove padding on exit
}

// add mode flag so can search for SUBDIR or a FILE == 0
CAT_LIST *tree_node(CAT_LIST *ccat,char *str,BYTE mode)
{
    int len;
    while(ccat != NULL)
    {
        len = strlen(ccat->name);
        if(strncmp(str,ccat->name,len)==0)
        {
            if(*(str+len ) == 0  && (ccat->flag & mode) == mode)  
               // exact match, end of target string, has correct mode
               return(ccat);
            else if ((ccat->flag & SUBDIR) && *(str+len) == DELIM &&
                 ccat->child != NULL)
               return(tree_node(ccat->child,str+len+1,mode));
        }
        ccat = ccat->next;
    }
    return(NULL); // not found
}

/* recursively look for next node 
   at first tried depth first, but it isn't really
   or wouldn't allow sub dir's at same level before those below
   see sample from ralf and notes in 4_dir.lst
   this routine tries to localize search from current node
   when hit ENDDIR
   Interesting, it can go down as may have added some directories
   below this.  An can scan next, but don't let it recursively
   scan up
*/
CAT_LIST * next_tree_node(CAT_LIST * ccat)
{
    CAT_LIST * rnode=NULL, *ncat; 
    // check for valid children after this node

    if(ccat != NULL && !(ccat->flag & EMPTYDIR) && (ccat->flag & SUBDIR))
    {
        if(ccat->child != NULL && 
           !(ccat->child->flag & EMPTYDIR) && (ccat->child->flag & SUBDIR))
           rnode = ccat->child;
        else // check for others after immediate child
           rnode = next_tree_node(ccat->child);
    }

    /* this node has no valid children, try next
       some serious questions about order here.
       apparently trace down children of next (depth first)
       rather than (breadth first) checking all next in current chain
    */
    if(rnode == NULL && ccat != NULL)
    {   
        ncat = ccat->next;
        while(ncat != NULL && rnode == NULL)
        {
            if(!(ncat->flag & EMPTYDIR) && (ncat->flag & SUBDIR))
            {   // check for children before using this node
                if((rnode = next_tree_node(ncat->child)) == NULL)
                    rnode = ncat; // use this as has no valid children
            }
            else
                ncat = ncat->next;            
        }
    }
    return(rnode);
}


CAT_LIST *new_cat_list(CAT_LIST *prev,char *name,long off,struct ms_dir_fixed *fixed)
{   char *str;
    CAT_LIST * new=NULL;
    if((str = strdup(name)) != NULL &&
       (new = malloc(sizeof(CAT_LIST))) != NULL)
    {
        new->name = str;
        new->parent = prev;
        new->child = new->next = NULL;
        new->data_off = off;
        if(fixed != NULL)
        {
           new->flag = fixed->flag;
           new->attrib = fixed->attrib;
           new->datetime = fixed->m_datetime;
           new->file_len = fixed->file_len;
        }
        else
        {
           new->flag = SUBDIR; // this is all to handle root, must be better way
           new->attrib = 0;
           new->datetime = 0;
           new->file_len = 0;
        }
    }
    return(new);       
}


void free_cat_list(CAT_LIST *root)
{
    CAT_LIST *this;
    while(root != NULL)
    {
         free_cat_list(root->child);
         this = root;
         root = root->next;
         free(this);
    }
}

/* extract one file. may be called from main via -x or from do_redirect()
   CAUTION if the file is compressed, it repeats the decompression cycle
   for EACH file.  If you have a lot of small files in a segment this is
   REDUNDANT.  Oh well.  Maybe you should decompress the entire file first.
*/
int do_extract(int fp,struct vtbl_ver *v,char *name,CAT_LIST *ccat)
{
    // extern references to stuff in decompression section which may get moved
    extern int fin,fout;
    extern long comp_wr;
    extern char *dcompfnm; // working file name
    int  i,j,rd,fo=EOF,ft=EOF,suc = 1,cnt=0;
    WORD sz; // temp store for segment size with RAW_SEG cleared
    char *suc_xmsg = "\nSuccessful extraction";
    FOFFSET soff=LSEEK_ERR,  // save start offset into working file dcompfnm after decompress
         cur_pos,  // position in input file
         tlng; 
    struct cseg_head shead;


    if(access(name,0) == 0)
    {
       printf("\nfile exits: %s\n overwrite (Y/N) ",name);
       if(bques(0) != 1)
              return(-1);
    }
    if ((fo=open(name,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,
                        S_IREAD|S_IWRITE)) == EOF)
         printf("\nfailed to open %s",name);
    else if((v->vtbl.comp & 0x80) == 0)  // not compressed, just grab it
    {
        fin = fp;
        fout = fo;
        if(lseek(fp,ccat->data_off+v->database,SEEK_SET) == LSEEK_ERR)
        {
            printf("\nseek failed");
            suc = -1;
        }
        else if((suc = copy_region(ccat->file_len)) != 0)
            printf("\nextract failed");
        else
            printf(suc_xmsg);
    }
    else // its compressed, need to decompress seg(s) of interest first
         // do limited range of logic in do_decompress()
    {
        if ((ft=open(dcompfnm,O_BINARY|O_RDWR|O_CREAT|O_TRUNC,
                        S_IREAD|S_IWRITE)) == EOF)
            printf("\nfailed to open working file: %s",dcompfnm);
        fin = fp;  // set up globals for decompression
        fout = ft;

        soff = cur_pos = v->database; // start of data section
        rd = 0;
        // loop through looking for segment(s) or interest
        while((tlng=lseek(fin,cur_pos,SEEK_SET)) == cur_pos &&
              (rd=read(fin,&shead,sizeof(shead))) == sizeof(shead))
        {
/*          either it is known to start in prior segment or
            prior is last segment need to process to find out for sure
*/
            if(ccat->data_off < shead.cum_sz ||
                 shead.seg_sz == 0) 
                 cnt++; 
            else if(cnt == 0) // update soff
                soff = cur_pos;
            if(ccat->data_off + ccat->file_len <= shead.cum_sz ||
                  shead.seg_sz == 0) // there are no more segments!
                break;  // know we got it all in prior segment
            sz = shead.seg_sz & ~RAW_SEG; // clear flag
            // add 6/01/07 for WIN95 decompress
            cur_pos += SEG_SZ;
            // cur_pos += sz+rd; // old code was WINME specific
            rd = 0; // be sure this gets updated
        } 
        if(cnt ==0)
        {
            printf("\nFailed to find segment(s) containing data");
            return(suc);
        }
        // now extract data region of interest, cnt segments to working file
        rd = 0;
        cur_pos = soff;
        soff = LSEEK_ERR;
        i = 0;
        while((tlng=lseek(fin,cur_pos,SEEK_SET)) == cur_pos &&
              (rd=read(fin,&shead,sizeof(shead))) == sizeof(shead))
        {
            sz = shead.seg_sz & ~RAW_SEG; // clear flag
            if(soff == LSEEK_ERR)  // get start offset in 1st working file seg
                 soff = ccat->data_off - shead.cum_sz;
            if(shead.seg_sz & RAW_SEG) // its not compressed
            {
               if((j = copy_region((long)sz)) != 0) 
                    break;
            }
            else
                decomp_seg();  // currently has no return!
            if(++i >= cnt)
                break; // do not clear rd, use as read(cseg_head) test
            // input file position is a little random, do a seek
            cur_pos += rd+sz; // where we want to be for next shead
            rd = 0; // be sure this gets updated
        }
        // depends on global comp_wr being set by copy_region & decomp_seg()
        // last seg needs special handling as shead.cum_sz = 0
        if(shead.cum_sz == 0 && shead.seg_sz == 0)
           shead.cum_sz = v->vtbl.dataSz[0];
        if(rd < sizeof(shead) || i < cnt ||
                ccat->data_off + ccat->file_len > shead.cum_sz+comp_wr)
            printf("\nabort: error decompressing data");
        else
        {
            printf("\nDecompressed %d segments to temp working file",cnt);
            fin = ft;
            fout = fo;
            if( (cur_pos = lseek(ft,soff,SEEK_SET)) != soff ||
                (suc = copy_region(ccat->file_len)) != 0)
                printf("\nextract from working file failed");
            else
                printf(suc_xmsg);
        }

    }
    if(fo != EOF)
    {
       close(fo);
       set_fattrib(name,ccat->datetime,ccat->attrib);
    }
    if(ft != EOF)
       close(ft);
    return(suc);
}


#define PATH_LEN 255
// make the following global to limit amount of stack space used in tree_copy()
char tree_path[PATH_LEN+1]; // make this global to simplify recursion stack
static struct stat sbuf;   // used by do_redirect() and tree_copy()
                          

/* this is a recursive copy routine
   copies all files in this subdir
   if term == '*' will also copy all files in all subdir's

   note uses global tree_path[PATH_LEN] to store file paths

ccat->flag & SUBDIR && !(ccat->flag & EMPTYDIR) 
*/
int tree_copy(int fp,struct vtbl_ver *v,CAT_LIST *ccat,int start,char term)
{
    int len,cnt=0,err;
    while(ccat != NULL)
    {
        err = 0;
        len = strlen(ccat->name);
        if(start+len >= PATH_LEN)
        {
            printf("\nbuffer PATH_LEN exceeded: %s%c%s",
               tree_path,DELIM,ccat->name);
            if(term == '*' || term == '+')
               fputc(DELIM,stdout); 
            err++;
        }
        else if(len > 0)
            strcpy(tree_path+start,ccat->name); // append file of sub dir name
        else
        {
            printf("\ndirectory name can not be empty");
            err++;
        }

        if(ccat->flag & SUBDIR) 
        {
            if(err == 0 && (term == '*' || term == '+')&& !(ccat->flag & EMPTYDIR))
            {
                if((err = stat(tree_path,&sbuf)) == 0 && // it exists
                    !(sbuf.st_mode & S_IFDIR) ) 
                {
                   printf("\nsub dir path used as file: %s",tree_path);
                   err++;
                }
                else if(term == '+') // try to create the directory
                {
#ifdef unix
                    // accessable & searchable
                    err = mkdir(tree_path,S_IREAD | S_IWRITE | S_IEXEC); 
#else
                    err = mkdir(tree_path);
#endif
                    if(err)
                        printf("\nmkdir(%s) failed",tree_path);
                }
                
                if(err == 0)
                {
                    // attempts to copy all sub directories
                    tree_path[start+len++] = DELIM;
                    cnt += tree_copy(fp,v,ccat->child,start+len,term);
                }
            }
            // else if term == DELIM ignore subdir
        }
        else // must be a file in this dir, extract it
           if(err == 0 &&
              (len=do_extract(fp,v,tree_path,ccat)) == 0)
              cnt++;
        ccat = ccat->next;
    }
    return(cnt);
}

/* step through list of paths
   find via tree_node()
   then step through copying file
*/
long do_redirect(int fp,struct vtbl_ver *v,CAT_LIST *root,PATH_ELEM paths[],int rcnt)
{
    CAT_LIST *ccat;
    PATH_ELEM *pe;
    char *des;
    int p,i,derr;
    long cnt=0; 
    printf("\nDoing Redirectable path based extract with %d source paths",rcnt);
    for(i=0;i<rcnt;i++)
    {
        pe = &paths[i];
        if(pe->redirect != NULL)
           des = pe->redirect; // use redirect path (may be empty)
        else
           des = pe->path; // use source path
        /* 
           1/20/04 try new system, named root for all trees, Win98 or Win95
        */
        ccat = tree_node(root,pe->path,SUBDIR);
        
        derr = 0;
        p = strlen(des);
        if(ccat == NULL)
           printf("\nsource path %d not found: %s",i,pe->path);
        else if(ccat->flag & EMPTYDIR || ccat->child == NULL)
           printf("\nsource directory empty: %s",pe->path);
        else if(p > 0 && ((derr = stat(des,&sbuf)) != 0 ||
                !(sbuf.st_mode & S_IFDIR) ) )
         //  if p == 0 we are using current directory, it better exist!
        {
            if(!(sbuf.st_mode & S_IFDIR))
            {
               printf("\nredirect path is not a subdir: %s",des);
               derr++;
            }
            else if(pe->term == '+') // try to create the directory
            {
#ifdef unix
                // accessable & searchable
                derr = mkdir(des,S_IREAD | S_IWRITE | S_IEXEC); 
#else
                derr = mkdir(des);
#endif
                if(derr)
                    printf("\nmkdir(%s) failed",des);
            }
        }

        if(derr)
           printf("\n   unavailable destination path: %s",des);
        else if(p+1 >= PATH_LEN)
           printf("\nabort, path buffer length exceeded");
        else if(ccat != NULL)
        {
           if(p > 0) // there is a des path, its not current directory
           {
              strcpy(tree_path,des); // global tree_path[] modified by tree_copy
              tree_path[p++] = DELIM;
           }
           cnt += tree_copy(fp,v,ccat->child,p,pe->term);
        }
    }
    return(cnt);
}


/* path remapping routines added 1/11/04
   handles quoted strings which are required for LFN access
*/
#define LN_SZ 255

int get_paths(char *fn,struct path_elem paths[],int sz)
{
    FILE *fp;
    int i,r=0,cnt = 0,ln=0,len;
    char quoted,*ch,line[LN_SZ+1];
    if((fp = fopen(fn,"r")) == NULL)
    {
       printf("\nfailed to open redirect command file: %s ",fn);
       return(-1); // open error
    }
    while(cnt < sz)
    {
        if(fgets(line,LN_SZ,fp) == NULL)
        {
           if(feof(fp))
             printf("\neof after line %d",ln);
           break;
        }
        else  
        {
           paths[cnt].term = 0; // validatation 
           quoted = 0;
           ln++;
           len = strlen(line);
           while(len > 0)
              if(line[len-1] == '\n' || line[len-1] == ' ')
                 line[--len] = 0;  // strip lf && trailing spaces so have string
              else
                 break;
           if(len <= 0 || line[0] == '#')
           {
              printf("\nskipping empty line %d",ln);
              continue;
           }

           ch = line;
           // skip leading blanks and possible quote
           while(*ch != 0 && *ch == ' ')
           {
              len--;
              ch++; 
           }

           // allocate room for trailing NUL
           if((paths[cnt].path = malloc(len+1)) == NULL)
           {
              printf("\nfatal alloc error");
              break;
           }
           i = 0;
           while(*ch != 0 && (quoted || *ch != ' ') && i < len)
           {
              if(*ch == '"')
                   quoted = quoted ^ 1;
              else
                   *(paths[cnt].path+ i++) = *ch;
              ch++;
           }

           if(i > 0)
              paths[cnt].term = *(paths[cnt].path+ --i); // has a last char, save it
           if(i == 0)
                printf("\nerror line %d source path has length 0!",ln);
           if(paths[cnt].term != DELIM && paths[cnt].term != '*' && 
              paths[cnt].term != '+')
           {
                printf("\nerror line %d in source path must end in %c, *, +",
                         ln,DELIM);
                i = -1;
           }
           else if (quoted)
           {
                printf("\nerror line %d unmatched quotes in source path",ln);
                i = -1;
           }
           
           if(i > 0)
           { 
               *(paths[cnt].path+ i++) = 0; // terminate source path

               while(*ch != 0 && *ch == ' ' && i < len) // find start next
               {
                  ch++;
               }


               if(*ch != 0 && i < len)
                   paths[cnt].redirect = paths[cnt].path+ i; // 2nd path exists
               else
                   paths[cnt].redirect = NULL; // not redirected
               while(*ch != 0 && (quoted || *ch != ' ') && i < len)
               {
                  if(*ch == '"')
                       quoted = quoted ^ 1;
                  else
                       *(paths[cnt].path+ i++) = *ch;
                  ch++;r++;
               }
               *(paths[cnt].path+ i) = 0;  // terminate redirect
               if(!(*ch == 0 || *ch == ' ') || quoted || r == 0)
               {
                   printf("\nerror line %d invalid redirect path %s",
                         ln,paths[cnt].redirect);
                   i = -1;
               }
               // check termination, will append strings to this.
               // for consistency must end with DELIM
               else if(*(paths[cnt].path+ i -1) != DELIM) 
               {
                   printf("redirect path must end with %c\n  %s",
                           DELIM,paths[cnt].redirect);
                   i = -1;
               }
               else
               {
                   if(strcmp(paths[cnt].redirect,"./") == 0 ||
                      strcmp(paths[cnt].redirect,".\\") == 0)
                      // its redirected to current directory, redirect path empty
                         paths[cnt].redirect = paths[cnt].path+ i;
                   // terminate string, r == strlen(paths[].redirect)
                   *(paths[cnt].path+ i-1) = 0; // delete DELIM
                   cnt++;
               }
           }
           if(i <= 0)
           {
                printf("\n  skipping: %s",line);
                free(paths[cnt].path);
                continue;
           }
        }
    }
    return(cnt); // # valid paths
}

