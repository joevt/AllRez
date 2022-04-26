#ifndef __LINUXTYPES_H__
#define __LINUXTYPES_H__

/*
	Types for drm_dp_helper.h
*/

typedef UInt8 u8;
typedef UInt16 u16;
typedef UInt32 u32;
typedef UInt64 u64;
typedef UInt16 __be16;

typedef struct mutex {
	UInt8 stuff;
} mutex;

typedef struct delayed_work {
	UInt8 stuff;
} delayed_work;

typedef struct i2c_adapter {
	UInt8 stuff;
} i2c_adapter;

typedef struct work_struct {
	UInt8 stuff;
} work_struct;

#define BIT(x) (1 << (x))

#define __packed __attribute__((packed))

#define IS_ENABLED(x) 0
#define IS_BUILTIN(x) 0
#define IS_MODULE(x) 0

#define CONFIG_DRM_AMD_DC_DCN

#endif
