/* ntbkup.h  see ntbkup.c and xpbkup.h
   original concepts developed in xpbkup.c
   Then discovered MTF100a.pdf and attempt to
   change over to MTF structures.

Sorry about the mess below with structure packing.
#pragma pack(1) is a little sloppy, but works well with 
   MSDOS, WIN32, and CYGWIN
However my Linux 2.4 gcc Version  doesn't like it
while CYGWIN's gcc Version 3.3.1 ignores this alternate approach
   __attribute__ ((packed)) for each structure
If someone can tell me why I'd like to know.
See the hidden -ss option in main to check your
structure sizes if you try another compiler.


time is an array of 5 bytes which is treated as
a bit field with the 1st byte being the MSB and the 5th
byte being the LSB.  Parsing from LSB to MSB
seconds  6 bits    range 0-59
minutes  6 bits    range 0-59
hour     5 bits    range 0-23
day      5 bits    range 1-31
month    4 bits    range 1-12
year    14 bits    range 0 - 16383

definately room after/in year for other info, but for
files I've looked at which were created in 2003, the value
of year has been 2003

see dos.h for DOS bitmap of  attribute byte.

12/29/03 copy from xpbkup.h go to MTF specific defines
     which were triggered by defining _MTF in prior work
01/01/04 add the FOFFSET logic from Msqic to allow
    4GB files under dos
01/02/04 add WIN32 logic for MSVC 5.0
01/17/04 neither MTF_DB_HDR nor MTF_FDD_VOLB needs PACKED
12/30/06 remove define MAX_PATH=25, its done in ntbkup.c and
         then passed as an argument to get_paths in nttree.c
         Can one modify on command line when compile ntbkup.c
*/

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;


#ifndef MSDOS // => _WIN32 or Linux
#define HAS_INT64
#define LSEEK_ERR (-1)  // this works for standard lseek stuff
#ifdef _WIN32
#pragma pack(1)
#define PACKED ;
typedef __int64 FOFFSET;
#define QWORD FOFFSET // close enough
#else  // Unix gcc assumed
typedef unsigned long long QWORD;
// return from lseek, or lseeki64 controlled via -D_FILE_OFFSET_BITS=64
typedef off_t FOFFSET;
#ifdef __CYGWIN__
#pragma pack(1)    // Linux gcc won't compile with this, see above
#define PACKED ;
#else
// to pack linux structures selectively
#define PACKED __attribute__ ((packed));
#define O_BINARY 0  // this Microsoft mode flag undefined in Linux gcc
#endif
#endif

#else  // its MSDOS 
// in DOS just pack everything
#pragma pack(1)
#define PACKED ;

#ifdef _4GB
typedef unsigned long FOFFSET;
#define LSEEK_ERR ((DWORD)-1L)
#else
typedef long FOFFSET;  // 2 GB is default std C
#define LSEEK_ERR (-1L)
#endif

#endif


#define XPBLK_SZ 0x400 // max blocking factor
#define PATH_LEN 255   // max path length in tree_path[]


/* had defined mode bits 0-3, through SILENT and tested
   for mode == 0 to display normal message.
   should now be mode & SILENT inhibits messages
   mode & VERBOSE gives additional messages
*/
#define EXTRACT 1
#define CATALOG 2
#define SILENT  4
#define VERBOSE 8
#define LPATH   0x10  // limit xtract based on path
#define TAGS    0x20
#define TREE    0x40
#define DTREE   0x80
#define XTREE   0x100


// index defines into array string for is_keyword() in ntbkup.c
// they make error checks generic, but must be updated if array changed
#define NUL_KEY (-2)  // returns if the long is == 0
#define TAPE 0
#define SSET 2
#define VOLB 3
#define DIRB 4
#define FILE_K 5  // FILE would conflict stdio.h
#define MTF_MAX_BLK 9
#define TFDD 11
#define NACL 12
#define NTQU 13
#define CSUM 14
#define STAN 15
#define SPAD 16
#define FEND 17
#define NTOI 18




// MTF defines
// string type:  id MTF_DB_HDR.str_type
#define NO_STRINGS 0
#define ANSI_STR  1
#define UNICODE_STR 2


typedef struct mtf_tape_adr {
WORD size,     // string length (its not nul terminated)
     offset;   // from start struct containing MTF_TAPE_ADR
} MTF_TAPE_ADR;

typedef struct mtf_db_hdr {
BYTE tag[4];
DWORD attrib;
WORD  var_sz;
BYTE  OS_id,OS_ver; // see MTF appendix A, {0xE,0x2} in samples
#ifdef HAS_INT64
QWORD disp_sz,  
      fmt_adr;
#else
DWORD disp_sz,  
      disp_sz_hi,
      fmt_adr,
      fmt_adr_hi;
#endif
BYTE  res_MBC[2],
      res[6];
DWORD blk_id;
BYTE  res2[4];
MTF_TAPE_ADR OS_data;
BYTE  str_type,
      res3;
WORD  cksum;
} MTF_DB_HDR;

// note the common header above embedded at start of each common block
struct mtf_tape {
MTF_DB_HDR head;
DWORD media_id,
      attrib;
WORD  media_seq,
      pass_encryp,
      soft_blk_sz,
      media_cat_type;
MTF_TAPE_ADR media_name,
      media_label,
      media_pass,
      software_name;
DWORD fmt_blk_sz,
      vendor_id;
BYTE  media_date[5],
      MTF_major_ver;
} PACKED

typedef struct mtf_tape MTF_TAPE;

struct mtf_volb {
MTF_DB_HDR head;
DWORD attrib;
MTF_TAPE_ADR dev_name,
             vol_name,
             mach_name;
BYTE date_written[5];
} PACKED

typedef struct mtf_volb MTF_VOLB;

typedef struct mtf_dirb {
MTF_DB_HDR head;
DWORD attrib;
// following are 4 times
BYTE modify[5],
     create[5],
     backup[5],
     access[5];
DWORD dir_id;
MTF_TAPE_ADR name; // includes trailing NUL => '/'
} MTF_DIRB;

struct mtf_sset {
MTF_DB_HDR head;
DWORD attrib;
WORD encrypt_alg,
     comp_alg,
     vendor_id,
     set_num;
MTF_TAPE_ADR set_name,
     set_descript,
     set_passwd,
     user_name;
#ifdef HAS_INT64
QWORD blk_adr;
#else
DWORD blk_adr,
      blk_adr_hi;
#endif
BYTE written[5], // date/time written
     sft_major_ver,
     sft_minor_ver,
     time_zone,
     MTF_minor_ver,
     media_cat_ver;
} PACKED
typedef struct mtf_sset MTF_SSET;

typedef struct mtf_file {
MTF_DB_HDR head;
DWORD attrib;
// following are 4 times
BYTE modify[5],
     create[5],
     backup[5],
     access[5];
DWORD dir_id,
      file_id;
MTF_TAPE_ADR name; 
} MTF_FILE;



// Stream Media Format Attributes
#define STREAM_CONTINUE 1 //This is a continuation stream. BIT0
#define STREAM_VARIABLE 2 //Data size for this stream is variable. BIT1
#define STREAM_VAR_END  4 //Last piece of the variable length data. BIT2
#define STREAM_ENCRYPTED 8 //This stream is encrypted. BIT3
#define STREAM_COMPRESSED 0x10 //This stream is compressed. BIT4
#define STREAM_CHECKSUMED 0x20 // checksum stream follows. BIT5
#define STREAM_EMBEDDED_LENGTH 0x40 //Stream length embedded in data. BIT6

struct mtf_stream_hdr {
BYTE tag[4];
WORD sys_attrib,
     media_attrib;
#ifdef HAS_INT64
QWORD length;
#else
DWORD length,
      length_hi;  // and you better hope its 0!
#endif
WORD  encrypt_alg,
      compress_alg,
      cksum;
} PACKED
typedef struct mtf_stream_hdr MTF_STREAM_HDR;


// embedded in TFDD stream, mini headers:
typedef struct mtf_fdd_hdr {
WORD length;
BYTE  tag[4];
WORD media_seq;
DWORD attrib; // of parent common block
#ifdef HAS_INT64
QWORD fmt_adr,  // maps to XPBLK_SZ block # in file (was blk in xpbkup.h)
      disp_sz;  // for a data file = file length, ie in MTF_FDD_FILE
#else
DWORD fmt_adr,
      fmt_adr_hi,
      disp_sz,
      disp_sz_hi;
#endif
long  link;
BYTE  os_id,
      os_ver,
      str_type,
      pad;
} MTF_FDD_HDR;  

struct mtf_fdd_volb {
MTF_FDD_HDR head;
DWORD attrib;  // distrinct from head.attrib!?
MTF_TAPE_ADR dev_name,
             vol_name,
             mach_name,
             OS_data;
BYTE date_written[5];
} PACKED 
typedef struct mtf_fdd_volb MTF_FDD_VOLB;

struct mtf_fdd_dirb {
MTF_FDD_HDR head;
// following are 4 date/times
BYTE modify[5],
     create[5],
     backup[5],
     access[5];
DWORD attrib;
MTF_TAPE_ADR name, // includes trailing NUL => '/'
     OS_data;
};

typedef struct mtf_fdd_dirb MTF_FDD_DIRB;
typedef struct mtf_fdd_dirb MTF_FDD_FILE;
// note MTF_FDD_FILE is currently identical to MTF_FDD_DIRB

/* The only place things become slightly unclear is with regard
   to the OS_data for these last two typedefs.
   Its OS ID and OS version specific.  I assume ID=14 for NT
   in this program, and use the Version 1 file structure.
   The current files I've seen are OS Version 2, but it doesn't seem
   to have changed.  There is a similar structure (without the flags field)
   for NT_DIRB which I am currently ignoring.
*/
typedef struct nt_file {
DWORD attrib; // low order byte matches MSDOS attribute byte
WORD offset,  // point to short file name unicode string
     size;
DWORD flags;
} NT_FILE;


//------------------- new tree routines for lookup -------------
/* for my dynamic link list of directory nodes
*/
typedef struct dir_list {
char *name;
BYTE key;   // VOLB or DIRB
FOFFSET data_off; // offset to DIRB record in file
struct dir_list * next,
                * child;  // only subdir has children
                // ignore parent, shouldn't need
} DIR_LIST;

// used in path remapping functions
typedef struct path_elem {
char term,
     *path,
     *redirect;
} PATH_ELEM;


