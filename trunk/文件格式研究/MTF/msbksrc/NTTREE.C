/* nttree.c  tree and path routines for ntbkup.c

Add overview below 3/17/04 as on review for version 1.03
I find I've forgotten the underlying logic.
The tree is created via calls to add_tree_node()
This dynamically allocates space for the new node
and if the full path string copied from global tree_path[].
It then calls insert_tree() to put the new node into tree
order in memory.  The 2/6/04 1.02 version of this seems to 
work correctly.

From ntbkup.h
typedef struct dir_list {
char *name;
BYTE key;   // VOLB or DIRB
FOFFSET data_off; // offset to DIRB record in file
struct dir_list * next,   // at same logical level
                * child;  // of current node, always points to a DIRB
                // ignore parent,used in msqic, shouldn't need
} DIR_LIST;

The intent is to allow seeking to the location of the
record in the archive using data_off.
If its a VOLB record next points to the next physical drive
on the system and child points to the directory tree on this drive.
name is the drive specification (or as of 5/17/05 network drive spec).

If its a DIRB record, next points to directories at the same
level, ie the chain representing all the subdirectories of the parent.
FILEs are not included in this tree.  One must seek to the
DIRB record in the archive and traverse the FILE data that
follows these records to get at FILE information.  See
path_extract() which calls do_file() once for each FILE found.

Each name in a DIRB record is the full path within its volume,
ie it does not include the drive specification "?:".
This is important for my restore logic as I allow redirection
where the beginning of the destination path may be specified
by the user and the remainder taken from the archives structure.

do_redirect() is the routine that handles the command line
options -p and @ in NTBKUP.  The -p is just a special case
where the user specifies one top level archive source node
and the extraction is done starting at the current level.
@ allows the user to specify multiple top level source nodes
and corresponding destination nodes in a command file.

do_redirect() makes one call to the recursive do_path()
routine for each top level archive node to be extracted.
If called via the @ option, prior to each call to do_path()
it sets the current directory to the desired destination directory.
do_path() is called with a parameter, par, which is the amount
of the source path to be masked when changing to the next
directory.

With hind site I'm not real comfortable with my use of
chdir() system calls to handle this.  It is easy to get out
of sync and cause errors.  A rewrite where the full paths
are used instead of relative paths is probably desirable.

find_tree() is the lookup routine, it can be entered at any
level in the tree and recursively searches from that point
down (through the children in the tree) for a full
case sensitive match with the supplied path string.
Per above, unless the tree is entered at to top level
the search path should NOT include a drive spec.

1/24/04 created, splitting earlier tree work out of ntbkup.c
1/25/04 this compiled and tst_path() with chg2path() seem
        to work with relative paths.  As initially written
        it assumed the gap between a parent/child was one
        level.  This is not the case.  Add logic to create
        children child by child in chg2path().

        Also change get_paths() a little more.  The concept of
        using the current directory just doesn't make sense
        with this implimentation as it did in MSQIC.

2/6/04 see do_redirect, play with l and lev
       there was a short int mismatch in calls to chg2path
3/16/04 fix error in chg2path when term == '+'
3/17/04 back up version above to nttree2.c  apparently it has
        problems with level stuff.  Make some significant changes.
        see nt_dan.c and dan_files.txt for some notes an testing.
        do_path() was heart of changes.
        some error messages added included skipped file
        count in path_extract() and skipped directories
        in do_redirect().
        tweak get_paths() to allow quoated string for destination

5/17/04 see emails from Peter Feldman <peter@offshorespars.com>
        turns out drive (ie VOLB spec) and be a network spec.
        change find_tree()
4/06/05 troubles with Jack's big archive, in add_tree_node()
        test or empty path, only allocated if plen > 0
        Its messy, maybe just shouldn't add to tree at all????
        at least warn now.

12/20/06 increase LN_SZ define from 255 to 600
*/

#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h> // for toupper()
#include <string.h> // define memset
#include <sys/types.h>
#include <sys/stat.h>   // for open() defines 

#if  defined(MSDOS) || defined(_WIN32) 
# ifdef MSDOS
#  define OS_STR "MSDOS"
# else
#  define OS_STR "WIN32"
# endif
#define DELIM '\\'
#include <direct.h>
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


#include "ntbkup.h"  // includes FOFFSET logic

// called in ntbkup.c to copy one file
FOFFSET do_file(int fp, FOFFSET soff,int mode); // uses global char *filter

// add some tree routines

// make the following global to limit amount of stack space used in tree_copy()
extern char tree_path[]; // see ntbkup.c
extern BYTE buf[];       //  "    "
static struct stat sbuf;   // used by do_redirect() and tree_copy()

// recursive: see insert_tree() this routine mimic that behavior to find a node
DIR_LIST *find_tree(DIR_LIST *next,char *str)
{
   int i,t;
   char ch;
   while(next != NULL)
   {
      i=0;
      if(next->name != NULL)  // test for no name, shouldn't happen, but...
      {
         while((t = *(next->name+i) - *(str+i)) == 0 && 
               (ch = *(str+i)) != 0)
            i++;
         if(*(next->name+i) == 0)
         {        
            if( t != 0)
            {
               if(next->child == NULL)
                   next = NULL;
               else
                   next = find_tree(next->child,str);
            }
            break; // we found it
         }
      }
      next = next->next;
   }
   return(next);
}

/* prior to 5/17/04 I looked for 1st DELIM and assumed VOLB name
   was everything ahead of this DELIM.  Works fine for C:
   but not network drive spec, ie \\ANDY\C
   so change to any match with Volume name
*/
DIR_LIST *find_path(DIR_LIST *next, char *str)
{
   int len,slen=strlen(str);
   while(next != NULL)   // find volume
   {
       if(next->name != NULL)  // add error check
       {
          len = strlen(next->name);
          if(strnicmp(str,next->name,len) == 0)
          {
              next = next->child;
              break;
          }
       }
       next = next->next;
   }
   // 2nd case below allows + or * to recursively do entire volume
   if(next == NULL || (slen-len == 1 && *(str+len) == DELIM))
      return(next); // NULL or top level node in a volume
   else
      return(find_tree(next,str+len));  // recursive search of a volumes tree
}

void disp_tree(DIR_LIST *d)
{
    while(d != NULL)
    {
      if(d->name == NULL) // add error check
         printf("\n    oops there was no name!");
      else if(d->key == VOLB)
         printf("\n%s",d->name);
      else
         printf("\n   %s",d->name);
      if(d->child != NULL)
         disp_tree(d->child);
      d = d->next;
    }
}

/* Make some assumptions.  Mainly it inserts top node first and additional ones
   will be below those in tree already.  therefore only check for children
   try if at end of current node name, the other is an actual child
   for now assume all end with a DELIM
   otherwise insert at end of next
*/

int insert_tree(DIR_LIST *next, DIR_LIST *node)
{

   int i,t,ldelim=0,suc=0,depth=1;
   char ch;
   while(next != NULL)
   {
      i=0;
      if(next->name != NULL && node->name != NULL)  // add error check
      {
      while((t = *(next->name+i) - *(node->name+i)) == 0 
             && (ch = *(node->name+i)) != 0)
      {
         if(t == 0 && ch == DELIM)
            ldelim = i;
         i++;
      } 
      if(*(next->name+i) == 0)
      {        
         if( t != 0)
         {
            if(next->child == NULL)
                next->child = node;
            else
                depth += insert_tree(next->child,node);
            break;
         }
         else
            printf("\nerror duplicate DIRB %s",next->name);
      }
      }
      if(next->next == NULL)
      {
         next->next = node;
         break;
      }
      next = next->next;
   }
   return(depth); // just for fun
}

int add_tree_node(DIR_LIST **root, FOFFSET off, int key)
{
    int suc = 0,plen = strlen(tree_path);
    DIR_LIST *node,*tmp;
    if((node = malloc(sizeof(DIR_LIST))) == NULL || 
       (plen > 0 && (node->name = malloc(plen+1)) == NULL))
    {
        suc++;
        printf("\nDIR Tree allocation error");
    }
    else
    {
        node->next = node->child = NULL;
        node->key = key;
        node->data_off = off;
        if(plen <= 0)
        {
           printf("\nWarning empty path in add_tree_node()");
           node->name = NULL;
        }
        else
        {
           strncpy(node->name,tree_path,plen);
           *(node->name+plen) = 0; // insure termination
        }

        tmp = *root;
        if(tmp != NULL) // find last node at drive level
           while(tmp->next != NULL)
              tmp = tmp->next;

        if(*root == NULL) // always want a VOLB for a drive at root level
        {
           if(key != VOLB)
            {
               printf("\nDIR Tree warning, 1st node not a VOLUME! force '?:'");
               strcpy(tree_path,"?:");
               if((suc = add_tree_node(root,(FOFFSET)0,VOLB)) != 0)
                  return(suc);
               else
                  tmp = *root; // we just forced a root
            }
            else
                *root = node;  // set 1st VOLB to root
        }
        else if(key == VOLB)
            tmp->next = node;  // its definately a volume spec, append

        if(key == DIRB) // in partial archive may hit DIRB with no VOLB
        {
            if(tmp->child == NULL)
                tmp->child = node;
            else
                insert_tree(tmp->child,node);
        }
        else if(key != VOLB)
        {
            printf("\ninvalid key %d",key);
            suc++;
        }
    }
    
    return(suc);
}


/* extract all files in DIRB block to current directory via do_file()
   note this means its filtered by global filter -x option sets
   3/18/04 find from Dan's stdout.txt that the algorithm below
   periodically hits a SPAD or CSUM key.  Think this happens
   if file ends near the XPBLK_SZ increment, the trialing stuff
   in the stream header is hit.  Used to quite if key != FILE_K
   add logic to ignore these false hits.  Also ignore if key < 0
   as DIRB often more the XPBLK_SZ bytes long so get some
   unrecognized hits in here.

   Returns a negative # on error, or # >=0 which is the number
   of files successfully extracted
*/
int path_extract(int fp, DIR_LIST *node,unsigned long *fskip)
{
    FOFFSET off,skip;
    int key,rd;
    unsigned int cnt=0;
    char dir_cnt=0;
    // seek to location of DIRB for directory in the archive
    if((off = (FOFFSET)lseek(fp,node->data_off,SEEK_SET)) != node->data_off)
        return(-1);
    // extract all FILE tags
    while((rd = read(fp,buf,XPBLK_SZ)) > 4)
    {
        skip = 0;
        key=is_keyword((char *)buf);
        if(dir_cnt > 0 && key >= 0 && key != FILE_K && key <= MTF_MAX_BLK)
             return(cnt); // have done all files in directory
        if(dir_cnt == 0) // only do this once on first pass
        {
            if(key != DIRB)
                return(-2); // should have landed on a dirb
            else
                dir_cnt++; // assume its the correct one!
        }
        if(key == FILE_K)
        {
            skip = do_file(fp,off,EXTRACT|SILENT);
            if(skip > 0 && skip != LSEEK_ERR)
               cnt++;
            else
               (*fskip)++; // extract failed
        }
        if(skip != LSEEK_ERR && skip > XPBLK_SZ)
            off += skip;
        else
            off += rd; // always tries to advance one block   

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
                   return(-1);
           }
        }
    }
    return(-2); // only get to here on read error
}

/* path remapping routines copied from msqicrcv.c 1/24/04
   handles quoted strings which are required for LFN access
   3/19/04 looks like delimiter check on destination saws '"' 
   instead of delim if path was quoated

   note 12/20/06 increase LN_SZ from 255
*/
#define LN_SZ 600


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
           if(len <= 0 || line[0] == '#') // allow comments in line
           {
              printf("\nskipping empty line %d",ln);
              continue;
           }

           ch = line;
           // skip leading blanks
           while(*ch != 0 && *ch == ' ')
           {
              len--;
              ch++; 
           }

           // allocate room for trailing NUL
           // both source and destination are in same allocation
           if((paths[cnt].path = malloc(len+2)) == NULL)
           {
              printf("\nfatal alloc error");
              break;
           }
           i = 0;
           while(*ch != 0 && (quoted || *ch != ' ') && i <= len)
           {
              if(*ch == '"')
                   quoted = quoted ^ 1;
              else
                   *(paths[cnt].path+ i++) = *ch;
              ch++;
           }

           if(i > 0)
           {
/*            below for msqic where do not want trialing delimiter
              paths[cnt].term = *(paths[cnt].path+ --i); // has a last char, save it
              change 1/24/04 for NTBKUP to insert terminator
*/
              paths[cnt].term = *(paths[cnt].path+ i-1); // has a last char, save it
              *(paths[cnt].path+ i -1) = DELIM; // replace with DELIM
           
           }
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

               while(*ch != 0 && *ch == ' ' && i <= len) // find start next
               {
                  ch++;
               }


               if(*ch != 0 && i <= len)
                   paths[cnt].redirect = paths[cnt].path+ i; // 2nd path exists
               else
                   paths[cnt].redirect = NULL; // not redirected
               while(*ch != 0 && (quoted || *ch != ' ') && i <= len)
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
               else if(*(paths[cnt].path+ i -1)  != DELIM) 
               {
                   printf("redirect path must end with %c",DELIM);
                   i = -1;
               }
               else
               {
                   // 1/25/04 remove empty string for current directory logic 
                   // in msqic.c I delete delimiter, leave for NTBKUP
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

/* set current directory based on path
   checks for a leading DELIM (only passed from do_redirect() with level == 0)
   to determine if its an absolute or relative path
   It may step down more than one directory level to new path
   The number of levels stepped down is returned

   1/28/04 change return, add parameter level that used to be return on success
   NOTE if term == '+' this will attempt to create directories
   return -1 on fatal error,
           1 on warning
   3/16/03 add reset of suc = 0 when term == '+' and
        create directory successfully after not finding it
        also always return suc, the return was undefined on a error!
   3/17/03 This routine passes back *level=0 if its the root
        else *level = # of subdir's created
             appears this is always 1 and we don't need a while loop!
*/
int chg2path(char *path,char term,WORD *level)
{
    int suc = 0,l = strlen(path),rd;
    char savch;
    *level=0;
    l--;  
    /* at this point *(path + l) should be the last DELIM
       however if its a path to the root this can also be the
       root path spec, ie l == 0.  This routine attempts to
       handle both absolute paths (start with DELIM) and relative
       paths that don't begin with DELIM.
       This routine aborts if:
       no path, length 1 but not root, or a subdir without trailing DELIM
    */
    if(l < 0 || (l == 0 && *path != DELIM) ||
       (l > 0 && *(path+l) != DELIM))
        suc = 1;
    else 
    {         
        while(suc == 0 && l > 0)
        {
            rd = 0;
            while(*(path+rd) != 0 && *(path+rd) != DELIM)
                rd++;
            if(rd > 0)
                *(path+rd) = 0; // overwrite all but 1st DELIM with 0, then restore below
            else if(*level == 0 && l > 0)
            {
                savch = *(path+rd+1); // allow absolute path to root at level 0
                *(path+rd+1) = 0;
            }
            if(rd > 0 )
            {
               suc = stat(path,&sbuf); // check status of directory
               if(term == '+') // try to create the sub dir(s) if not there
               {
                    if(suc == 0 && // directory exists
                       !(sbuf.st_mode & S_IFDIR) ) // as a file
                       suc = -1; // fatal error
                    else if (suc) // it doesn't exist try to create
                    {
#ifdef unix
                       // accessable & searchable
                       if(mkdir(path,S_IREAD | S_IWRITE | S_IEXEC) != 0)
                          suc = -1;
#else
                       if(mkdir(path) != 0)
                          suc = -1;
#endif
                       else
                          suc = 0; // clear error from test for directory above
                    } 
               }
               else if(suc || !(sbuf.st_mode & S_IFDIR))
                    suc = 1; // just a warning, its no a directory
            }
            if(suc == 0 && chdir(path)) // finally try to go there
               suc = -1;
            if(rd > 0)
            {
                *(path+l) = DELIM; // restore
                (*level)++; // don't increment for root
            }
            else if(*level == 0 && l > 0)
                *(path+rd+1) = savch;
            rd++; // skip the DELIM
            path+=rd; // advance to next node
            l -= rd;
        }
    }
    if(suc != 0)
         printf("\nFailed: chdir(%s)",path);
    return(suc);
}

 
/*  called from do_redirect() only if recursive extract
    attempts to step through the nested paths
    and extract files with a call to path_extract() for each sub directory

    4/17/04 think was messed up
    a) its when level == 0 that there is no parent, had logic reversed
    b) did initialize lev, and didn't pass new nested level to 
       recursive do_path() cause of how I used level.  Change.
       if there is no child need lev = 0 so don't step back up
    c) add a chdir("..") before chg2dir() while getting next
       at end.  Getting fuzzy, but looks right
    d) move display of node->name and its len inside while loop
       so each node displayed and len, which is critical to chg2dir()
       and do_path(), gets updated as move between directories at the
       same level.
    e) add lev == 1 test and error message on failure to do child 
    f) add skip parameter to track if directories skipped
*/
short do_path(int fp,DIR_LIST *node,WORD level,WORD *dskip,unsigned long *fskip,short par,char term)
{
   short suc = 0,len;
   short lev;
   while(suc == 0 && node != NULL)
   {
      printf("\nlevel %d: %s",level,node->name);
#ifndef TEST // define to test directory routines without file extraction   
      if((len=path_extract(fp,node,fskip)) < 0) // get all files for this node
      {
          printf("\nfatal error while extracting files from this directory");
          (*dskip)++;
      }

#endif
      len=strlen(node->name);
      lev = 0; // assume don't change level
      if(node->child != NULL)
      { 
         if(strlen(node->child->name) > len &&
            chg2path(node->child->name+len,term,&lev) == 0 && lev == 1)
                suc = do_path(fp,node->child,level+lev,dskip,fskip,len,term);
         else
         {
            printf("\nskipping unreachable source child node lev %d:\n  %s",
                    lev,node->child->name);
            (*dskip)++;
         }
         /* prior to 1/28/04 was fatal error if chg2path failed
            now continue, just skip this directory
            This allows term '*' to use directory existance as a filter
         */
      }
      
      if(level == 0)
         break; // there is no parent for start node
      // do_path() returns to current directory level
      // if not level 0, restore to parent level so chg2dir(next->node) will work
      else if(suc == 0 && chdir("..") != 0)
             suc = -2;
      else   // its not the start node, has a valid parent
      {
         while(suc==0) 
         { // look for a valid next directory
             if((node = node->next) == NULL)
                 break; // no more nodes
             if(par >= strlen(node->name))
             {
                 printf("\nskip, invalid next node: %s",node->name);
                (*dskip)++;
             }
             else if(chg2path(node->name+par,term,&lev) == 0)
             {   if(lev == 1)
                    break; // positioned correctly
                 else
                 {
                    printf("\nwarning can't reach next source node lev %d:\n   %s",
                           lev,node->name);
                    while(lev > 0 && chdir("..") == 0)
                         lev--; // attempt to recover
                    if(lev != 0) 
                        suc = -2; // fatal if can't
                    else
                        (*dskip)++;
                 }
             }
         } 
      }
   }

   if(suc == -2)
         printf("\nfatal error returning to parent dir");
    return(suc);
}

/* step through path list extracting files
   one call to do_path() for each entry in paths[] array
       This routine makes each top level redirected path the current directory.
       It handles drive parsing, routines below this assume current drive.

*/
int do_redirect(int fp, DIR_LIST *root,struct path_elem *paths,int rcnt)
{
   int i,l,drvn,suc=0;
   WORD lev,dskip;
   unsigned long fskip=0;
   char *drv,*path,doit;
   DIR_LIST *node,*parent=NULL;
   printf("\nDoing redirect:");
   for(i=0;i<rcnt;i++)
   {
       printf("\nSource: %s",paths->path);
       if(paths->redirect != NULL)
          printf("\nredirect: %s",paths->redirect);
       if((node = find_path(root,paths->path)) == NULL)
       {
           printf("\ncan't find source path");
           continue;
       }
       else
       {
          suc = 0;
#ifndef unix // its MSDOS or WIN32 so has physical drives
          if(paths->redirect == NULL)
              path = paths->path;
          else
              path = paths->redirect;

          l = 0;
          drv = path;
          while(*path != 0 && *path != DELIM)  
          {
              path++;
              l++;
          }
          if(l && *path == DELIM) // there is a drive spec
          {  // temp terminator for drive spec
             *path = 0;
             strupr(drv);
             if(l != 2 || *(drv+1) != ':' || (drvn = *drv - 'A') < 0)
                suc = 1;
             else // DOS WIN32 specific A: is 1, C: is 3 etc
                suc = _chdrive(*drv-'A'+1);  // failed to change drive
             if(suc)
             {
                 continue;
                 printf("\nfailed to select drive: %s",drv);
             }
             *path = DELIM; // restore
          }
#else
          // need to redirect physical drive to a mount point
          if(paths->redirect == NULL)
          {
              printf("\nSKIP, for Unix redirect path must be defined or current dir");
              continue;
          }
          else
              path = paths->redirect;
#endif 
          // now see if we can select the directory
          doit = 1;
          if((l=strlen(path)) == 0 && rcnt > 1)
          {
             printf("\n Warning, empty path not allowed with multiple sources, its unpredictable!"); 
             doit = 0;
          }
          if(doit && (l == 0 || chg2path(path,paths->term,&lev) == 0) )
          {  // chg2path handles directory creation if there is a path
             fskip = dskip = 0; // initialize
             if(paths->term == DELIM) // just this directory
             {
                if((l = path_extract(fp,node,&fskip)) > 0)
                {
                  printf("\n%d files extracted from this directory",l);
                  l = 0; // so no error message below
                }                   
             }
             else // do nested directories
             {
                l = do_path(fp,node,0,&dskip,&fskip,0,paths->term);
             }
             // no error check, an error in one path does NOT terminate extact
             if(l != 0)
                 printf("\n  extract aborted, fatal error parsing this path");
             if(dskip != 0)
                 printf("\n  warning %u subdirectories were skipped",dskip);
             if(fskip != 0)
                 printf("\n  warning %lu files were skipped",fskip);
          }
        
       }
       paths++;
   }
   return(suc); 
}




