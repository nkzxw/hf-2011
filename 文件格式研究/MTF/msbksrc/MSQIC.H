/* defines for msqic work
   made separate file 10/14/03

   need __attribute__ ((packed))
   for linux gcc

note the qic_vtbl seems ok without PACKED, but cseg_head requires!

see section 7.1.3 of QIC 113 Spec for this info, below works in qic80.c
in *.qic there are two related structures which I named ms_qic_fix*

struct dir_fixed {
BYTE len,   // of the rest of the record + vendor data
     attrib;
DWORD datetime,
     size;   // file size in bytes + data header size, or 0 for empty dir
BYTE extra;  // flag byte containing file info
// min length is 0xA, ie include extra but no vendor data
} __attribute__ ((packed));

above not what Win9x MSBackUp uses! 
also note EDAT_SIG and DAT_SIG only occur in data region not dir set

11/31/03 add some bitmap defines and .err field in struct dir_blk
12/14/03
add field struct cseg_head *pseg;
    in struct dir_blk
12/16/03 per Ralf Westram's input try to clean up some of
signed unsigned issues in DWORD verus long for file offsets
If I use as offset should probably be a long!
in struct vtbl_ver change .database and .dirbase
12/22/03 try some FOFFSET logic
 change struct vtbl_ver offset fields back to DWORD

1/7/04 add Ralf's EMPTYDIR define
1/11/04 change params to do_extract()
   remove #define SEARCH 2   may never have been used...
   update tree_node() adding BYTE mode argumnet
   add do_redirect() prototype
1/20/04 add DELIM definition for path separator
1/22/04 change args for do_extract()
4/28/04 add RECREAT to indicate the original VTBL was invalid
    Also requires WIN_MASK and change value of WIN95 and WINME
    Need in do_decompress() if attempt to reconstruct archive
    Note there are a couple places where I should be using
    FOFFSET instead of DWORD, I've left them alone for ease
    in displaying data, but see vtbl_ver and seg_head

    add mk_date() prototype
    add define for VTBL_DATE_FUDGE
*/

#if defined(MSDOS) || defined(_WIN32)
#pragma pack(1)
// default for MSVC _WIN32 should be byte packing, but need pack(1)!
#define PACKED ;
#define DELIM '\\'
#else // Unix
#define DELIM '/'
#ifdef __CYGWIN__    // note  O_BINARY is defined
#pragma pack(1)    // Linux gcc won't compile with this, see above
#define PACKED ;
#else
// to pack linux structures selectively
#define PACKED __attribute__ ((packed));
#define O_BINARY 0  // this Microsoft mode flag undefined in Linux gcc
#endif
#endif

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#ifdef _4GB
typedef unsigned long FOFFSET;
#define LSEEK_ERR ((DWORD)-1L)
#else
  #ifdef HAS_INT64  // force this manually for 64 bit access
// for now WIN32 specific
  typedef __int64 FOFFSET;
  #define lseek _lseeki64  // redirect to 64 bit access routine
  #define LSEEK_ERR (-1)  // this works for standard lseek stuff
  #else
  typedef long FOFFSET;  // 2 GB is default std C
  #define LSEEK_ERR (-1L)
  #endif
#endif

#define SEG_SZ 29696 // MSBackUP wants data and dir segs to be multiple of this
// in compressed file each segment including catalog start with cseg_head


// from pp9 of QIC113G, 
struct qic_vtbl {
BYTE tag[4]; // should be 'VTBL'
DWORD nseg; // # of logical segments
char  desc[44];
DWORD date; // date and time created
BYTE flag;  // bitmap
BYTE seq;   // multi catridge sequence #
WORD rev_major,rev_minor; // revision numberrs
BYTE vres[14]; // reserved for vendor extensions
DWORD start,end; // physical QFA block numbers
    /* In Win98 & ME subtract 3 from above for zero based SEGMENT index
       to start first data segment and start first directory segment
    */
BYTE passwd[8]; // if not used, start with a 0 byte
DWORD dirSz,     // size of file set directory region in bytes
      dataSz[2]; // total size of data region in bytes
BYTE OSver[2];   // major and minor #
BYTE sdrv[16];   // source drive volume lable
BYTE ldev,       // logical dev file set originated from
     res,        // should be 0
     comp,       // compression bitmap, 0 if not used
     OStype,
     res2[2];       // more reserved stuff
} PACKED




/* If its a compressed volume there will be cseg_head
   records ahead of each segment.  The first immediately
   follows the Volume Table.  
   For the sake of argument, lets assume per QIC133 segments are
   supposed to be < 32K, ie seg_sz high order bit isn't required.
   So its a flag bit, set to indicate raw data?  IE do not 
   decompress this segment.  Use seg_sz to jump to the
   next segment header.
*/
#define RAW_SEG 0x8000  // flag for a raw data segment

struct cseg_head {
DWORD cum_sz,   // cumlative uncompressed bytes at end this segment
      cum_sz_hi;// normally zero. High order DWORD of above for > 4Gb
WORD  seg_sz;   // physical bytes in this segment, offset to next header
}  PACKED


struct ms_dir_fixed {
WORD rec_len;   // only valid in dir set or Win95 Data region
DWORD ndx[2];   // thought this was quad word pointer to data? apparently not
                // ndx[0] varies, ndx[1] = 0, was unknow[8]  
                // in data section always seems to be 0xffffffff
WORD path_len,  // @ 0xA  # path chars, exits in catalog and data section
                // however path chars only present in data section
     unknww1;   // 0xA  always?
BYTE flag;      //  flag bytes
WORD unknww2;   // 0x7  always?
DWORD file_len; // @ 0x11 # bytes in original file
BYTE unknwb1[20],  // was flags[0x18] but attrib at flags[20]
     attrib,
     unknwb2[3];   
DWORD c_datetime, // created
      unknwl1,    // always 0xFFFFFFFF?
      a_datetime, // accessed
      unknwl2,    // always 0xFFFFFFFF?
      m_datetime, // modified, as shown in DOS
      unknwl3;    // so can be expanded? always 0xFFFFFFFF?
WORD nm_len; // length of the long variable length name
}  PACKED

// contains var length name, case sensitive, unicode

struct ms_dir_fixed2 {
BYTE unkwn1[13];   // was [0x15];  this region fairly constant
DWORD var1;        // these vars change file to file
DWORD var2;
WORD  nm_len; // length of 2nd, short ie DOS, variable length name
} PACKED

#ifdef MSDOS
#pragma pack()  // don't read rest from file so don't care
#endif

// var length name, always upper case => DOS, unicode
// if in data region path follows, not in directory set
//     var length path per ms_dir_fixed.path_len, unicode


// use to return VTBL header and version detection from get_vtbl()
#define WIN95   1
#define WINME   3  // used for 98 and ME, ie had an MDID
#define RECREAT 0x80  // original VTBL corrupt, recreated
#define WIN_MASK 0x7

// tmode defines passed to disp_tree()
#define S_SEGS  1  // show segments in tree
#define DIRONLY 2
// bitmap flags for control flags in main and next_dir()
#define EXTRACT 2  // used in main and msqicrcv.c
#define TREE    4  
// below only used in msqicrcv.c:do_recover( mode)
// but W95 and W98 set in main?
#define QUERY 1
#define W95     4  // not used in same flag as TREE so no conflict
#define W98     8

struct vtbl_ver {
BYTE ver,  
     cnt,  // # of regions
     ndx;  // which VTBL when more than one (1 based!)
DWORD database; // start of data segment
DWORD dirbase;  // start of directory segment
DWORD  flen;     // file length
struct qic_vtbl vtbl; // copy of vtbl of interest
};


/* use to return data from next_dir() routine
   Not pointers are to buffer locations which can be over written
   if care not taken after call!
*/
struct dir_blk {
char err;
struct ms_dir_fixed *fix1;
BYTE *nm1;
struct ms_dir_fixed2 *fix2;
BYTE *nm2,
     *path;
struct cseg_head *pseg; // NULL if NOT compressed, otherwise points to buffer
};

/* for my dynamic link list of nodes
   order of subdirs in msbkup2.qic
   root|DIRLAST,temp,dos,csource
   could save entire record, but not now....
*/
typedef struct cat_list {
char *name;
BYTE  flag,  // its a subdir (maybe only one) or file
      attrib; // file attributes
FOFFSET data_off; // offset into uncompressed data region for start
DWORD datetime, // m_datetime -> dos time, ie modified
      file_len; // @ 0x11 # bytes in original file
struct cat_list * next,
                * child,  // only subdir has children
                * parent;
} CAT_LIST;

// used in path remapping functions
typedef struct path_elem {
char term,
     *path,
     *redirect;
} PATH_ELEM;

#define MAX_PATH 25  // max PATH_ELEM array allocation

// bitmap flags for ctrl in find_seg
#define DISPLAY 1
#define FIND_POS 2  // probably not used...

/* Bitmap defines for flags
   flags below work with my QIC tape images, see qic80.c
   msc seems to do this differently
#define SUBDIR  0x20  // this is a directory entry, not a file
#define DIRLAST 0x40  // last entry in this directory
#define DIREND  0x80  // last entry in entire volume directory

   below seem to work with my current ms_dir_fixed.flags[2]
   Thanks to Ralf Westram for identifing this flag
   Given the unused bits, there are more to be identified!
*/
#define ISFILE    0    // no bit set for file, this define for readability
#define SUBDIR  0x1  // this is a directory entry, not a file
#define EMPTYDIR 0x2  // this marks an empty subdirectory
#define DIRLAST 0x8  // last entry in this directory
#define DIREND  0x30  // last entry in entire volume directory

#define DAT_SIG 0x33CC33CCL // signature at start of Data Segment
#define EDAT_SIG 0x66996699L // just before start of data file
#define UDEF_SIG 0xFFFFFFFFL // undefined DWORDS in Data Segment

#define VTBL_DATE_FUDGE   63072000L // date fudge for vtbl date


// add some function prototypes
// following in msqic.c
int  next_dir(int fp,struct dir_blk *ptr, BYTE *buf,WORD bsz,BYTE mode);
// following in msqicrcv.c
int get_paths(char *fn,struct path_elem paths[],int sz);
void disp_tree(CAT_LIST *ccat,int level,BYTE mode);
CAT_LIST *tree_node(CAT_LIST *ccat,char *str,BYTE mode);
CAT_LIST *next_tree_node(CAT_LIST * ccat);
CAT_LIST *new_cat_list(CAT_LIST *prev,char *name,long off,struct ms_dir_fixed *fixed);
void free_cat_list(CAT_LIST *root);
int do_extract(int fp,struct vtbl_ver *v,char *nm,CAT_LIST * ccat);
long do_redirect(int fp,struct vtbl_ver *v,CAT_LIST *root,PATH_ELEM paths[],int rcnt);
WORD do_recover(int fp,DWORD dataoff,DWORD dataend, BYTE *buf,WORD bsz,char *target);
int write_vtbl(char *qicnm,char *datnm);
// following in qicdcomp.c, see sloppy use of globals, sorry
int copy_region(long len);
void decomp_seg();  
int do_decompress(int fi,int fo,struct vtbl_ver *cvtbl);
unsigned long mk_date(char *str);