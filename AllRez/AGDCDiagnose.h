//
//  AGDCDiagnose.h
//  AllRez
//
//  Created by joevt on 2022-06-04.
//

#ifndef AGDCDiagnose_h
#define AGDCDiagnose_h

#include <IOKit/IOKitLib.h>

#define PACKED __attribute__((packed)) __attribute__((aligned(1)))

typedef uint64_t reginfo;

typedef struct {
	reginfo fb;
	uint64_t fbindex;
} fbinfo; // 16 bytes

typedef struct {
/*     0 */ uint16_t gpuid;
/*     2 */ uint16_t filler;
/*     4 */ uint32_t flags;
/*
					   2:"XG"
					0x10:"IG"
					0x20:"DG"
					0x40:"NG"
					   1:"eject"
					   4:"ejectFinalizing"
					   8:"ejectFinalized"
			   0x1000000:"unsupported"
			  0x80000000:"published"
			   0x8000000:"driversStarted"
			   0x4000000:"hasGPUC"
			  0x40000000:"terminated"
			  0x20000000:"quiet"
			  0x10000000:"pubSched"
			   0x2000000:"pubArmed"
				 default:"bit%d"
*/
/*     8 */ reginfo agdc;
/*  0x10 */ reginfo pci;
/*  0x18 */ uint64_t fbcount;
/*  0x20 */ fbinfo fb[32];
/* 0x220 */ reginfo accel;
/* 0x228 */ reginfo agdpclient;
/* 0x230 */ reginfo gpuc;
} subgpuinfo; // 0x238 bytes

typedef struct PACKED {
/*    0 */ char name[0x28];
/* 0x28 */ uint32_t state; // exists when not 0
/* 0x2c */ uint32_t managerState;
/* 0x30 */ uint32_t vendorclass;
/* 0x34 */ uint32_t vendorid;
/* 0x38 */ uint32_t vendorversion;
/* 0x3c */ uint8_t filler;
} launcherinfo; // 0x3d bytes

typedef struct PACKED {
/*     0 0x238 */ uint32_t index;
/*     4 0x23c */ uint32_t state; // 0:skip, 1:"Discovered", 2:"GPUCDetect", 3:"GPUCStartDrivers", 4:"WaitForDrivers", 5:"Published", 6:"Unpublished", 7:"Gone", 8:"Eject", 0x80:"GPUCDriverStartFailed", default:"unknown"
/*     8 0x240 */ uint64_t filler;
/*  0x10 0x248 */ uint8_t events; // 1:terminated default:""
/*  0x11 0x249 */ PACKED subgpuinfo subgpu;
/* 0x249 0x481 */ uint32_t dispPolicyState; // 0:"Pending", 1:"Managed", 2:"Unmanaged", 3:"Stopped", 4:"InitFailed", 5:"InitAborted", 6:"WaitingForAGDP", 7:"StartingManager", default:"unknown"
/* 0x24d 0x485 */ uint32_t dispPolicyLaunchIndex;
} gpuinfo; // 0x251 bytes

typedef struct PACKED {
	uint64_t timestamp;
	uint64_t object;
	uint16_t line;
	uint16_t tag0;
	uint16_t tag1;
	uint16_t tag2;
	uint64_t data0;
	uint64_t data1;
	uint64_t data2;
} gtraceentry;

typedef struct {
	uint32_t gtraceid[4]; // 0x67547261, 0x63654461, 0x74614475, 0x6d700000   'gTra' 'ceDa' 'taDu' 'mp\x00\x00'
	uint32_t dataVersion; // 0x102
	uint32_t maxEntries; // 512
	uint32_t counter; // 106
	uint8_t version[4]; // 6,5,7,0 = 6.5.6
	uint64_t hdrSize; // 56
	uint64_t ussue;
	uint64_t usct;
} gtraceheader;

typedef struct {
	gtraceheader header;
	gtraceentry entry[512];
} gtraceinfo;

typedef struct {
/*      0 */ char boardID[32];
/*   0x20 */ uint64_t started;
/*   0x28 */ uint64_t running;
/*   0x30 */ uint64_t featureMask;
/*   0x38 */ uint64_t platformFlags;
/*   0x40 */ uint64_t extraSupportFlags;
/*   0x48 */ uint64_t wranglerFlags;
/*   0x50 */ PACKED launcherinfo launcher[8];
/*  0x238 */ PACKED gpuinfo gpu[8];
/* 0x14c0 */ gtraceinfo gtrace;
/* 0x74f8 */ // end
} DisplayPolicyState;

#ifdef __cplusplus
extern "C" {
#endif
void DoDisplayPolicyState(void);
#ifdef __cplusplus
}
#endif

#endif /* AGDCDiagnose_h */
