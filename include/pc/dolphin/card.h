/**
 * @file card.h
 * @brief Stub Memory Card API for PC port
 *
 * The PC port uses file-based saves instead of memory cards.
 * This provides the basic types and stub declarations.
 */
#ifndef PC_DOLPHIN_CARD_H
#define PC_DOLPHIN_CARD_H

#include "pc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CARD_FILENAME_MAX 32
#define CARD_MAX_FILE 127
#define CARD_ICON_MAX 8

typedef void (*CARDCallback)(s32 chan, s32 result);

typedef struct CARDFileInfo {
    s32 chan;
    s32 fileNo;
    s32 offset;
    s32 length;
    u16 iBlock;
} CARDFileInfo;

typedef struct CARDDir {
    u8 gameName[4];
    u8 company[2];
    u8 _padding0;
    u8 bannerFormat;
    u8 fileName[CARD_FILENAME_MAX];
    u32 time;
    u32 iconAddr;
    u16 iconFormat;
    u16 iconSpeed;
    u8 permission;
    u8 copyTimes;
    u16 startBlock;
    u16 length;
    u8 _padding1[2];
    u32 commentAddr;
} CARDDir;

typedef struct CARDStat {
    char fileName[CARD_FILENAME_MAX];
    u32 length;
    u32 time;
    u8 gameName[4];
    u8 company[2];
    u8 bannerFormat;
    u32 iconAddr;
    u16 iconFormat;
    u16 iconSpeed;
    u32 commentAddr;
    u32 offsetBanner;
    u32 offsetBannerTlut;
    u32 offsetIcon[CARD_ICON_MAX];
    u32 offsetIconTlut;
    u32 offsetData;
} CARDStat;

/* Simplified CARDControl - just enough for compilation */
typedef struct CARDControl {
    s32 attached;
    s32 result;
    u16 size;
    u16 pageSize;
    s32 sectorSize;
    /* Skip GameCube-specific fields */
} CARDControl;

typedef struct CARDDecParam {
    u8* inputAddr;
    u32 inputLength;
    u32 aramAddr;
    u8* outputAddr;
} CARDDecParam;

typedef struct CARDID {
    u8 serial[32];
    u16 deviceID;
    u16 size;
    u16 encode;
    u8 padding[470];
    u16 checkSum;
    u16 checkSumInv;
} CARDID;

/* Size constants */
#define CARD_WORKAREA_SIZE (5 * 8 * 1024)
#define CARD_READ_SIZE 512
#define CARD_COMMENT_SIZE 64
#define CARD_ICON_WIDTH 32
#define CARD_ICON_HEIGHT 32
#define CARD_BANNER_WIDTH 96
#define CARD_BANNER_HEIGHT 32

#define CARD_NUM_CHANS 2

/* Result codes */
#define CARD_RESULT_UNLOCKED      1
#define CARD_RESULT_READY         0
#define CARD_RESULT_BUSY         -1
#define CARD_RESULT_WRONGDEVICE  -2
#define CARD_RESULT_NOCARD       -3
#define CARD_RESULT_NOFILE       -4
#define CARD_RESULT_IOERROR      -5
#define CARD_RESULT_BROKEN       -6
#define CARD_RESULT_EXIST        -7
#define CARD_RESULT_NOENT        -8
#define CARD_RESULT_INSSPACE     -9
#define CARD_RESULT_NOPERM      -10
#define CARD_RESULT_LIMIT       -11
#define CARD_RESULT_NAMETOOLONG -12
#define CARD_RESULT_ENCODING    -13
#define CARD_RESULT_CANCELED    -14
#define CARD_RESULT_FATAL_ERROR -128

/* Icon/Banner format flags */
#define CARD_STAT_ICON_NONE     0
#define CARD_STAT_ICON_C8       1
#define CARD_STAT_ICON_RGB5A3   2
#define CARD_STAT_ICON_MASK     3

#define CARD_STAT_BANNER_NONE   0
#define CARD_STAT_BANNER_C8     1
#define CARD_STAT_BANNER_RGB5A3 2
#define CARD_STAT_BANNER_MASK   3

#define CARD_STAT_ANIM_LOOP     0x00
#define CARD_STAT_ANIM_BOUNCE   0x04
#define CARD_STAT_ANIM_MASK     0x04

#define CARD_STAT_SPEED_END     0
#define CARD_STAT_SPEED_FAST    1
#define CARD_STAT_SPEED_MIDDLE  2
#define CARD_STAT_SPEED_SLOW    3
#define CARD_STAT_SPEED_MASK    3

#define CARD_ATTR_PUBLIC   0x04u
#define CARD_ATTR_NO_COPY  0x08u
#define CARD_ATTR_NO_MOVE  0x10u
#define CARD_ATTR_GLOBAL   0x20u
#define CARD_ATTR_COMPANY  0x40u

/* Macros */
#define CARDGetBannerFormat(stat) (((stat)->bannerFormat) & CARD_STAT_BANNER_MASK)
#define CARDGetIconAnim(stat) (((stat)->bannerFormat) & CARD_STAT_ANIM_MASK)
#define CARDGetIconFormat(stat, n) (((stat)->iconFormat >> (2 * (n))) & CARD_STAT_ICON_MASK)
#define CARDGetIconSpeed(stat, n) (((stat)->iconSpeed >> (2 * (n))) & CARD_STAT_SPEED_MASK)
#define CARDSetBannerFormat(stat, f) ((stat)->bannerFormat = (u8)(((stat)->bannerFormat & ~CARD_STAT_BANNER_MASK) | (f)))
#define CARDSetIconAnim(stat, f) ((stat)->bannerFormat = (u8)(((stat)->bannerFormat & ~CARD_STAT_ANIM_MASK) | (f)))
#define CARDSetIconFormat(stat, n, f) ((stat)->iconFormat = (u16)(((stat)->iconFormat & ~(CARD_STAT_ICON_MASK << (2 * (n)))) | ((f) << (2 * (n)))))
#define CARDSetIconSpeed(stat, n, f) ((stat)->iconSpeed = (u16)(((stat)->iconSpeed & ~(CARD_STAT_SPEED_MASK << (2 * (n)))) | ((f) << (2 * (n)))))
#define CARDSetIconAddress(stat, addr) ((stat)->iconAddr = (u32)(addr))
#define CARDSetCommentAddress(stat, addr) ((stat)->commentAddr = (u32)(addr))
#define CARDGetFileNo(fileInfo) ((fileInfo)->fileNo)

/* Encoding */
#define CARD_ENCODE_ANSI 0u
#define CARD_ENCODE_SJIS 1u

/* Mode */
#define CARD_MODE_NORMAL 0
#define CARD_MODE_FAST   1

/* Stub function declarations - will be implemented for file I/O */
void CARDInit(void);
s32 CARDCheck(s32 chan);
s32 CARDCheckAsync(s32 chan, CARDCallback callback);
s32 CARDCreate(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo);
s32 CARDCreateAsync(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo, CARDCallback callback);
s32 CARDDelete(s32 chan, const char* fileName);
s32 CARDDeleteAsync(s32 chan, const char* fileName, CARDCallback callback);
s32 CARDFastDelete(s32 chan, s32 fileNo);
s32 CARDFastDeleteAsync(s32 chan, s32 fileNo, CARDCallback callback);
s32 CARDFastOpen(s32 chan, s32 fileNo, CARDFileInfo* fileInfo);
s32 CARDFormat(s32 chan);
s32 CARDFormatAsync(s32 chan, CARDCallback callback);
s32 CARDFreeBlocks(s32 chan, s32* byteNotUsed, s32* filesNotUsed);
s32 CARDGetAttributes(s32 chan, s32 fileNo, u8* attr);
s32 CARDGetEncoding(s32 chan, u16* encode);
s32 CARDGetMemSize(s32 chan, u16* size);
s32 CARDGetResultCode(s32 chan);
s32 CARDGetSectorSize(s32 chan, u32* size);
s32 CARDGetSerialNo(s32 chan, u64* serialNo);
s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat* stat);
s32 CARDGetXferredBytes(s32 chan);
s32 CARDMount(s32 chan, void* workArea, CARDCallback detachCallback);
s32 CARDMountAsync(s32 chan, void* workArea, CARDCallback detachCallback, CARDCallback attachCallback);
s32 CARDOpen(s32 chan, const char* fileName, CARDFileInfo* fileInfo);
int CARDProbe(s32 chan);
s32 CARDProbeEx(s32 chan, s32* memSize, s32* sectorSize);
s32 CARDRename(s32 chan, const char* oldName, const char* newName);
s32 CARDRenameAsync(s32 chan, const char* oldName, const char* newName, CARDCallback callback);
s32 CARDSetAttributes(s32 chan, s32 fileNo, u8 attr);
s32 CARDSetAttributesAsync(s32 chan, s32 fileNo, u8 attr, CARDCallback callback);
s32 CARDSetStatus(s32 chan, s32 fileNo, CARDStat* stat);
s32 CARDSetStatusAsync(s32 chan, s32 fileNo, CARDStat* stat, CARDCallback callback);
s32 CARDUnmount(s32 chan);
s32 CARDGetCurrentMode(s32 chan, u32* mode);
s32 CARDCancel(CARDFileInfo* fileInfo);
s32 CARDClose(CARDFileInfo* fileInfo);
s32 CARDRead(CARDFileInfo* fileInfo, void* addr, s32 length, s32 offset);
s32 CARDReadAsync(CARDFileInfo* fileInfo, void* addr, s32 length, s32 offset, CARDCallback callback);
s32 CARDWrite(CARDFileInfo* fileInfo, const void* addr, s32 length, s32 offset);
s32 CARDWriteAsync(CARDFileInfo* fileInfo, const void* addr, s32 length, s32 offset, CARDCallback callback);
int CARDGetFastMode(void);
int CARDSetFastMode(int enable);

#ifdef __cplusplus
}
#endif

#endif /* PC_DOLPHIN_CARD_H */
