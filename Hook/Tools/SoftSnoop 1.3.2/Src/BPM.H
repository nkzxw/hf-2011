//
// some debug register flags
//


#ifndef __BPM_H__
#define __BPM_H__

// Dr6
#define BPM0_DETECTED                    0x00000001
#define BPM1_DETECTED                    0x00000002
#define BPM2_DETECTED                    0x00000004
#define BPM3_DETECTED                    0x00000008

// Dr7
#define BPM0_LOCAL_ENABLED               0x00000001
#define BPM0_W                           0x00010000
#define BPM0_RW                          0x00030000

#define BPM1_LOCAL_ENABLED               0x00000004
#define BPM1_W                           0x00100000
#define BPM1_RW                          0x00300000

#define BPM2_LOCAL_ENABLED               0x00000010
#define BPM2_W                           0x01000000
#define BPM2_RW                          0x03000000

#define BPM3_LOCAL_ENABLED               0x00000040
#define BPM3_W                           0x10000000
#define BPM3_RW                          0x30000000

#define BPM_LOCAL_EXACT                  0x00000100

#endif