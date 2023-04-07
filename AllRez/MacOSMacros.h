#ifndef __MACOSMACROS_HPP__
#define __MACOSMACROS_HPP__

#include <TargetConditionals.h>
#include <AvailabilityMacros.h>
#include <AssertMacros.h>

#ifndef MAC_OS_X_VERSION_10_0
#define MAC_OS_X_VERSION_10_0 1000
#endif

#ifndef MAC_OS_X_VERSION_10_1
#define MAC_OS_X_VERSION_10_1 1010
#endif

#ifndef MAC_OS_X_VERSION_10_2
#define MAC_OS_X_VERSION_10_2 1020
#endif

#ifndef MAC_OS_X_VERSION_10_3
#define MAC_OS_X_VERSION_10_3 1030
#endif

#ifndef MAC_OS_X_VERSION_10_3_9
#define MAC_OS_X_VERSION_10_3_9 1039
#endif

#ifndef MAC_OS_X_VERSION_10_4
#define MAC_OS_X_VERSION_10_4 1040
#endif

#ifndef MAC_OS_X_VERSION_10_5
#define MAC_OS_X_VERSION_10_5 1050
#endif

#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif

#ifndef MAC_OS_X_VERSION_10_7
#define MAC_OS_X_VERSION_10_7 1070
#endif

#ifndef MAC_OS_X_VERSION_10_8
#define MAC_OS_X_VERSION_10_8 1080
#endif

#ifndef MAC_OS_X_VERSION_10_9
#define MAC_OS_X_VERSION_10_9 1090
#endif

#ifndef MAC_OS_X_VERSION_10_10
#define MAC_OS_X_VERSION_10_10 101000
#endif

#ifndef MAC_OS_X_VERSION_10_10_2
#define MAC_OS_X_VERSION_10_10_2 101002
#endif

#ifndef MAC_OS_X_VERSION_10_10_3
#define MAC_OS_X_VERSION_10_10_3 101003
#endif

#ifndef MAC_OS_X_VERSION_10_11
#define MAC_OS_X_VERSION_10_11 101100
#endif

#ifndef MAC_OS_X_VERSION_10_11_2
#define MAC_OS_X_VERSION_10_11_2 101102
#endif

#ifndef MAC_OS_X_VERSION_10_11_3
#define MAC_OS_X_VERSION_10_11_3 101103
#endif

#ifndef MAC_OS_X_VERSION_10_11_4
#define MAC_OS_X_VERSION_10_11_4 101104
#endif

#ifndef MAC_OS_X_VERSION_10_12
#define MAC_OS_X_VERSION_10_12 101200
#endif

#ifndef MAC_OS_X_VERSION_10_12_1
#define MAC_OS_X_VERSION_10_12_1 101201
#endif

#ifndef MAC_OS_X_VERSION_10_12_2
#define MAC_OS_X_VERSION_10_12_2 101202
#endif

#ifndef MAC_OS_X_VERSION_10_12_4
#define MAC_OS_X_VERSION_10_12_4 101204
#endif

#ifndef MAC_OS_X_VERSION_10_13
#define MAC_OS_X_VERSION_10_13 101300
#endif

#ifndef MAC_OS_X_VERSION_10_13_1
#define MAC_OS_X_VERSION_10_13_1 101301
#endif

#ifndef MAC_OS_X_VERSION_10_13_2
#define MAC_OS_X_VERSION_10_13_2 101302
#endif

#ifndef MAC_OS_X_VERSION_10_13_4
#define MAC_OS_X_VERSION_10_13_4 101304
#endif

#ifndef MAC_OS_X_VERSION_10_14
#define MAC_OS_X_VERSION_10_14 101400
#endif

#ifndef MAC_OS_X_VERSION_10_14_1
#define MAC_OS_X_VERSION_10_14_1 101401
#endif

#ifndef MAC_OS_X_VERSION_10_14_4
#define MAC_OS_X_VERSION_10_14_4 101404
#endif

#ifndef MAC_OS_X_VERSION_10_15
#define MAC_OS_X_VERSION_10_15 101500
#endif

#ifndef MAC_OS_X_VERSION_10_15_1
#define MAC_OS_X_VERSION_10_15_1 101501
#endif

#ifndef MAC_OS_VERSION_12_1
#define MAC_OS_VERSION_12_1 120100
#endif

#ifndef MAC_OS_VERSION_12_2
#define MAC_OS_VERSION_12_2 120200
#endif

#ifndef MAC_OS_VERSION_12_3
#define MAC_OS_VERSION_12_3 120300
#endif

#if !defined(AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER)
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_1
#elsif !defined(verify_action)
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_2
#else
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_3
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_3_9
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER) && defined(TARGET_CPU_PPC64)
    #undef MAC_OS_X_VERSION_SDK
    #define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_4
	// 10.4u sdk
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER) && defined(TARGET_OS_EMBEDDED)
    #undef MAC_OS_X_VERSION_SDK
    #define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_5
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER)
    #undef MAC_OS_X_VERSION_SDK
    #define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_6
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER)
    #undef MAC_OS_X_VERSION_SDK
    #define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_7
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_8
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_9
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_10
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_10_2
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_10_3
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_11
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_11_2
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_11
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_12
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_13_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_13
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_13_1_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_13_1
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_13_2_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_13_2
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_14_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_14
#endif

#if defined(AVAILABLE_MAC_OS_X_VERSION_10_15_AND_LATER)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_15
#endif

#if defined(__MAC_10_15_1)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_X_VERSION_10_15_1
#endif

#if defined(MAC_OS_VERSION_11_0)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_11_0
#endif

#if defined(MAC_OS_VERSION_11_1)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_11_1
#endif

#if defined(MAC_OS_VERSION_11_2)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_11_2
#endif

#if defined(MAC_OS_VERSION_11_3)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_11_3
#endif

#if defined(MAC_OS_VERSION_12_0)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_12_0
#endif

#if defined (__MAC_12_1)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_12_1
#endif

#if defined (__MAC_12_2)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_12_2
#endif

#if defined (__MAC_12_3)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_12_3
#endif

#if defined(MAC_OS_VERSION_13_0)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_13_0
#endif

#if defined (__MAC_13_1)
	#undef MAC_OS_X_VERSION_SDK
	#define MAC_OS_X_VERSION_SDK MAC_OS_VERSION_13_1
	#define MAX_TESTED_MAC_OS_X_VERSION_SDK
#endif

#ifndef MAC_OS_VERSION_11_0
#define MAC_OS_VERSION_11_0 110000
#endif

#ifndef MAC_OS_VERSION_11_1
#define MAC_OS_VERSION_11_1 110100
#endif

#ifndef MAC_OS_VERSION_11_2
#define MAC_OS_VERSION_11_2 110200
#endif

#ifndef MAC_OS_VERSION_11_3
#define MAC_OS_VERSION_11_3 110300
#endif

#ifndef MAC_OS_VERSION_12_0
#define MAC_OS_VERSION_12_0 120000
#endif

#ifndef MAC_OS_VERSION_13_0
#define MAC_OS_VERSION_13_0 130000
#endif

#ifndef MAC_OS_VERSION_13_1
#define MAC_OS_VERSION_13_1 130100
#endif

#if !defined(MAC_OS_X_VERSION_MAX_ALLOWED) || !defined(MAC_OS_X_VERSION_SDK)
    #error missing #include AvailabilityMacros.h
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if defined(__ppc64__)
	#define ARCHITECTURE "ppc64"
	#ifndef __clang__
	#warning ppc64
	#endif
#elif defined(__ppc__)
	#define ARCHITECTURE "ppc"
	#ifndef __clang__
	#warning ppc
	#endif
#elif defined(__x86_64__)
	#define ARCHITECTURE "x86_64"
	#ifndef __clang__
	#warning x86_64
	#endif
#elif defined(__i386__)
	#define ARCHITECTURE "i386"
	#ifndef __clang__
	#warning i386
	#endif
#elif defined(__arm64e__)
	#define ARCHITECTURE "arm64e"
	#ifndef __clang__
	#warning arm64e
	#endif
#elif defined(__arm64__)
	#define ARCHITECTURE "arm64"
	#ifndef __clang__
	#warning arm64
	#endif
#else
	#define ARCHITECTURE "unknownArch"
    #error other architecture
#endif

#pragma message ARCHITECTURE " min:" STR(MAC_OS_X_VERSION_MIN_REQUIRED) " max:" STR(MAC_OS_X_VERSION_MAX_ALLOWED) " sdk:" STR(MAC_OS_X_VERSION_SDK)

#define API_OR_SDK_AVAILABLE_BEGIN(SDK, name) { { { if (&name != NULL) {
#define API_OR_SDK_AVAILABLE_END } } } }

#if defined(__has_builtin)
	#if __has_builtin(__builtin_available)

// __builtin_available only works down to Mac OS X 10.6 because 10.5 SDK is missing __isOSVersionAtLeast.

		#ifdef __cplusplus
		extern "C" {
		#endif
		int __isOSVersionAtLeast(int Major, int Minor, int Subminor);
		#ifdef __cplusplus
		}
		#endif

/*
		#undef API_OR_SDK_AVAILABLE_BEGIN
		#define API_OR_SDK_AVAILABLE_BEGIN(SDK, name) { if (!&__isOSVersionAtLeast) goto __label_ ## name; { if (__builtin_available(macOS SDK, *)) { \
__label_ ## name: if (&name) {
*/
 
	#endif
#endif

#endif /* __MACOSMACROS_H__ */
