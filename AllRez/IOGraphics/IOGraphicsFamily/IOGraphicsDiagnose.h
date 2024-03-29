//
//  IOGraphicsDiagnose.h
//  IOGraphics
//
//  Created by bparke on 12/16/16.
//
//

#ifndef IOGraphicsDiagnose_h
#define IOGraphicsDiagnose_h

#if defined(IOGD530_14)
#include "GTraceTypes.h"

#define IOGRAPHICS_DIAGNOSE_VERSION             6
#else
#define IOGRAPHICS_DIAGNOSE_VERSION             9
#endif

#define IOGRAPHICS_MAXIMUM_REPORTS              16
#if !defined(IOGD530_14)
#define IOGRAPHICS_MAXIMUM_FBS                  96
#else
#define IOGRAPHICS_TOKENBUFFERSIZE              (kGTraceMaximumLineCount * (sizeof(sGTrace) / sizeof(uint64_t))) // ensure >= kGTraceMaximumLineCount


// Client Interfaces
#define kIOGSharedInterface_IOGDiagnose         0
#define kIOGSharedInterface_ReservedB           1
#define kIOGSharedInterface_ReservedC           2
#define kIOGSharedInterface_ReservedD           3
#define kIOGSharedInterface_ReservedE           4

#define IONotify _ionotify
#define IOGReport _iogreport
#define IOGDiagnose _iogdiagnose
#endif

// stateBits
#define kIOGReportState_Opened                  (1 << 0)
#define kIOGReportState_Usable                  (1 << 1)
#define kIOGReportState_Paging                  (1 << 2)
#define kIOGReportState_Clamshell               (1 << 3)
#define kIOGReportState_ClamshellCurrent        (1 << 4)
#define kIOGReportState_ClamshellLast           (1 << 5)
#define kIOGReportState_SystemDark              (1 << 6)
#define kIOGReportState_Mirrored                (1 << 7)
#define kIOGReportState_SystemGated             (1 << 8)
#define kIOGReportState_WorkloopGated           (1 << 9)
#define kIOGReportState_NotificationActive      (1 << 10)
#define kIOGReportState_ServerNotified          (1 << 11)
#define kIOGReportState_ServerState             (1 << 12)
#define kIOGReportState_ServerPendingAck        (1 << 13)  // unused FBv6
#define kIOGReportState_PowerPendingChange      (1 << 14)
#define kIOGReportState_PowerPendingMuxChange   (1 << 15)
#define kIOGReportState_PrivateInvalid          (1 << 16)
#define kIOGReportState_ControllerInvalid       (1 << 17)
#define kIOGReportState_GraphicsWorkloopInvalid (1 << 18)
#define kIOGReportState_SystemWorkloopInvalid   (1 << 19)
#define kIOGReportState_SystemPowerAckTo        (1 << 20)
#define kIOGReportState_Online                  (1 << 21)
#define kIOGReportState_ClamshellOffline        (1 << 22)
#define kIOGReportState_Muted                   (1 << 23)
#define kIOGReportState_IsMuxSwitching          (1 << 24)


// External API states
#define kIOGReportAPIState_CreateSharedCursor               (1 << 0)
#define kIOGReportAPIState_GetPixelInformation              (1 << 1)
#define kIOGReportAPIState_GetCurrentDisplayMode            (1 << 2)
#define kIOGReportAPIState_SetStartupDisplayMode            (1 << 3)
#define kIOGReportAPIState_SetDisplayMode                   (1 << 4)
#define kIOGReportAPIState_GetInformationForDisplayMode     (1 << 5)
#define kIOGReportAPIState_GetDisplayModeCount              (1 << 6)
#define kIOGReportAPIState_GetDisplayModes                  (1 << 7)
#define kIOGReportAPIState_GetVRAMMapOffset                 (1 << 8)
#define kIOGReportAPIState_SetBounds                        (1 << 9)
#define kIOGReportAPIState_SetNewCursor                     (1 << 10)
#define kIOGReportAPIState_SetGammaTable                    (1 << 11)
#define kIOGReportAPIState_SetCursorVisible                 (1 << 12)
#define kIOGReportAPIState_SetCursorPosition                (1 << 13)
#define kIOGReportAPIState_AcknowledgeNotification          (1 << 14)
#define kIOGReportAPIState_SetColorConvertTable             (1 << 15)
#define kIOGReportAPIState_SetCLUTWithEntries               (1 << 16)
#define kIOGReportAPIState_ValidateDetailedTiming           (1 << 17)
#define kIOGReportAPIState_GetAttribute                     (1 << 18)
#define kIOGReportAPIState_SetAttribute                     (1 << 19)
#define kIOGReportAPIState_WSStartAttribute                 (1 << 20)
#define kIOGReportAPIState_EndConnectionChange              (1 << 21)
#define kIOGReportAPIState_ProcessConnectionChange          (1 << 22)
#define kIOGReportAPIState_RequestProbe                     (1 << 23)
#define kIOGReportAPIState_Close                            (1 << 24)
#define kIOGReportAPIState_SetProperties                    (1 << 25)
#define kIOGReportAPIState_CopySharedCursor                 (1 << 26)
#define kIOGReportAPIState_SetHibernateGammaTable           (1 << 27)
#define kIOGReportAPIState_SetNotificationPort              (1 << 28)
#define kIOGReportAPIState_RestoreCDBlob                    (1 << 29)


#pragma pack(push, 4)
typedef struct _iostamp {
    char            name[64];
    uint64_t        start;
    uint64_t        end;
    uint32_t        lastEvent;
} IOStamp;

typedef struct IONotify {
    uint64_t        groupID;
    IOStamp         stamp[IOGRAPHICS_MAXIMUM_REPORTS];
} IONotify;

typedef struct IOGReport {
    uint32_t        stateBits;
    uint32_t        pendingPowerState;

    uint32_t        notificationGroup;
    uint32_t        externalAPIState;

    uint64_t        systemOwner;
    uint64_t        workloopOwner;
    uint64_t        notificationOwner;
    uint64_t        regID;

    uint32_t        dependentIndex;
    uint32_t        wsaaState;

    uint32_t        systemGatedCount;
    uint32_t        workloopGatedCount;

    char            objectName[64];
#if defined(IOGD530_14) || defined(IOGD530_66) || defined(IOGD576_1)
    char            framebufferName[8];
#else
    char            framebufferName[64];
#endif

    IONotify        notifications[IOGRAPHICS_MAXIMUM_REPORTS];

    uint32_t        lastSuccessfulMode;
    uint32_t        aliasID;

#if defined(IOGD530_14)
    uint64_t        reservedC[15];
#else
    uint32_t        lastWSAAStatus;

    uint32_t        reservedA;
    uint64_t        reservedB[14];
#endif
} IOGReport;

typedef struct IOGDiagnose {
    // Kernel to User
    uint64_t        version;
    uint64_t        framebufferCount;

    uint32_t        length;
#if defined(IOGD530_14)
    uint32_t        _reservedA;
    IOGReport       fbState[IOGRAPHICS_MAXIMUM_REPORTS];

    uint32_t        _reservedB[8];

    uint64_t        systemBootEpochTime;
    uint32_t        tokenLine;
    uint32_t        tokenLineCount;
    uint32_t        tokenSize;
    uint64_t        tokenBuffer[IOGRAPHICS_TOKENBUFFERSIZE];
#else
    uint32_t        _reservedB[7];

    uint64_t        systemBootEpochTime;

    IOGReport       fbState[IOGRAPHICS_MAXIMUM_FBS];
#endif
} IOGDiagnose;
#pragma pack(pop)


#endif /* IOGraphicsDiagnose_h */
