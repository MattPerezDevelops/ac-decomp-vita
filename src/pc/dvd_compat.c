/*
 * DVD I/O Compatibility Layer for PC Port
 *
 * Maps GameCube DVD file operations to PC file I/O.
 * GameCube paths like "/forest_1st.arc" are mapped to:
 *   assets/files/forest_1st.arc  (preferred)
 *   orig/GAFE01_00/files/forest_1st.arc  (fallback)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "dolphin/dvd.h"
#include "dolphin/os.h"

/* Debug logging */
#define DVD_DEBUG 1

#if DVD_DEBUG
#define DVD_LOG(fmt, ...) printf("[DVD] " fmt "\n", ##__VA_ARGS__)
#else
#define DVD_LOG(fmt, ...)
#endif

/* Asset search paths (in priority order) */
static const char* asset_paths[] = {
    "assets/files/",                    /* Preferred: extracted assets */
    "orig/GAFE01_00/files/",           /* Fallback: US version */
    "../orig/GAFE01_00/files/",        /* Parent directory fallback */
    NULL
};

/* File handle tracking */
#define MAX_DVD_FILES 32

typedef struct {
    FILE* fp;
    char path[256];
    u32 size;
    int in_use;
} DVDFileHandle;

static DVDFileHandle dvd_files[MAX_DVD_FILES];
static int dvd_initialized = 0;

/* Simple file entry table for DVDConvertPathToEntrynum */
typedef struct {
    const char* path;
    int entry_num;
} FileEntry;

/* Known file entries - add more as needed */
static FileEntry file_entries[] = {
    { "/forest_1st.arc", 1 },
    { "/forest_2nd.arc", 2 },
    { "/audiorom.img", 3 },
    { "/foresta.rel.szs", 4 },
    { "/famicom.arc", 5 },
    { "/opening.bnr", 6 },
    { "/static.map", 7 },
    { "/static.str", 8 },
    { "/COPYDATE", 9 },
    { "forest_1st.arc", 1 },
    { "forest_2nd.arc", 2 },
    { "audiorom.img", 3 },
    { NULL, -1 }
};

/* DVDCheckDisk - Check if disc is ready */
BOOL DVDCheckDisk(void) {
    return TRUE;  /* Always ready on PC */
}

/* Initialize DVD subsystem */
void DVDInit(void) {
    if (dvd_initialized) return;

    memset(dvd_files, 0, sizeof(dvd_files));
    dvd_initialized = 1;

    DVD_LOG("DVD subsystem initialized");
    DVD_LOG("Asset search paths:");
    for (int i = 0; asset_paths[i] != NULL; i++) {
        DVD_LOG("  %d: %s", i, asset_paths[i]);
    }
}

/* Find a free file handle slot */
static int find_free_handle(void) {
    for (int i = 0; i < MAX_DVD_FILES; i++) {
        if (!dvd_files[i].in_use) {
            return i;
        }
    }
    return -1;
}

/* Build PC path from GameCube path */
static int build_pc_path(const char* gc_path, char* pc_path, size_t pc_path_size) {
    const char* filename = gc_path;

    /* Skip leading slash */
    if (filename[0] == '/') {
        filename++;
    }

    /* Try each asset path */
    for (int i = 0; asset_paths[i] != NULL; i++) {
        snprintf(pc_path, pc_path_size, "%s%s", asset_paths[i], filename);

        struct stat st;
        if (stat(pc_path, &st) == 0) {
            DVD_LOG("Found: %s -> %s (size: %ld)", gc_path, pc_path, (long)st.st_size);
            return (int)st.st_size;
        }
    }

    DVD_LOG("NOT FOUND: %s", gc_path);
    return -1;
}

/* DVDOpen - Open a file by path */
BOOL DVDOpen(char* filename, DVDFileInfo* fileInfo) {
    if (!dvd_initialized) {
        DVDInit();
    }

    DVD_LOG("DVDOpen(\"%s\")", filename);

    if (!filename || !fileInfo) {
        DVD_LOG("  ERROR: null parameter");
        return FALSE;
    }

    /* Find free handle */
    int handle = find_free_handle();
    if (handle < 0) {
        DVD_LOG("  ERROR: no free file handles");
        return FALSE;
    }

    /* Build PC path */
    char pc_path[512];
    int size = build_pc_path(filename, pc_path, sizeof(pc_path));
    if (size < 0) {
        return FALSE;
    }

    /* Open file */
    FILE* fp = fopen(pc_path, "rb");
    if (!fp) {
        DVD_LOG("  ERROR: fopen failed for %s", pc_path);
        return FALSE;
    }

    /* Store handle info */
    dvd_files[handle].fp = fp;
    strncpy(dvd_files[handle].path, pc_path, sizeof(dvd_files[handle].path) - 1);
    dvd_files[handle].size = (u32)size;
    dvd_files[handle].in_use = 1;

    /* Initialize DVDFileInfo */
    memset(fileInfo, 0, sizeof(DVDFileInfo));
    fileInfo->startAddr = (u32)handle;  /* Store handle index here */
    fileInfo->length = (u32)size;
    fileInfo->cb.state = DVD_STATE_END;

    DVD_LOG("  SUCCESS: handle=%d, size=%u", handle, size);
    return TRUE;
}

/* DVDFastOpen - Open by entry number */
BOOL DVDFastOpen(s32 entryNum, DVDFileInfo* fileInfo) {
    DVD_LOG("DVDFastOpen(entry=%d)", entryNum);

    /* Find path for entry number */
    for (int i = 0; file_entries[i].path != NULL; i++) {
        if (file_entries[i].entry_num == entryNum) {
            return DVDOpen((char*)file_entries[i].path, fileInfo);
        }
    }

    DVD_LOG("  ERROR: unknown entry number %d", entryNum);
    return FALSE;
}

/* DVDClose - Close a file */
BOOL DVDClose(DVDFileInfo* fileInfo) {
    if (!fileInfo) return FALSE;

    int handle = (int)fileInfo->startAddr;
    DVD_LOG("DVDClose(handle=%d)", handle);

    if (handle < 0 || handle >= MAX_DVD_FILES) {
        DVD_LOG("  ERROR: invalid handle");
        return FALSE;
    }

    if (!dvd_files[handle].in_use) {
        DVD_LOG("  WARNING: handle not in use");
        return TRUE;
    }

    if (dvd_files[handle].fp) {
        fclose(dvd_files[handle].fp);
    }

    dvd_files[handle].fp = NULL;
    dvd_files[handle].in_use = 0;

    DVD_LOG("  SUCCESS");
    return TRUE;
}

/* DVDReadPrio - Read from file */
s32 DVDReadPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, s32 prio) {
    (void)prio;  /* Priority not used on PC */

    if (!fileInfo || !addr) {
        DVD_LOG("DVDReadPrio: null parameter");
        return DVD_RESULT_FATAL_ERROR;
    }

    int handle = (int)fileInfo->startAddr;

    if (handle < 0 || handle >= MAX_DVD_FILES || !dvd_files[handle].in_use) {
        DVD_LOG("DVDReadPrio: invalid handle %d", handle);
        return DVD_RESULT_FATAL_ERROR;
    }

    FILE* fp = dvd_files[handle].fp;
    if (!fp) {
        DVD_LOG("DVDReadPrio: null file pointer");
        return DVD_RESULT_FATAL_ERROR;
    }

    DVD_LOG("DVDReadPrio(handle=%d, len=%d, off=%d)", handle, length, offset);

    /* Seek to offset */
    if (fseek(fp, offset, SEEK_SET) != 0) {
        DVD_LOG("  ERROR: seek failed");
        return DVD_RESULT_FATAL_ERROR;
    }

    /* Read data */
    size_t read = fread(addr, 1, (size_t)length, fp);

    DVD_LOG("  Read %zu bytes", read);

    fileInfo->cb.transferredSize = (u32)read;

    return (s32)read;
}

/* DVDReadAsyncPrio - Async read (we do it synchronously) */
BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset,
                      DVDCallback callback, s32 prio) {
    s32 result = DVDReadPrio(fileInfo, addr, length, offset, prio);

    if (callback) {
        callback(result, fileInfo);
    }

    return (result >= 0) ? TRUE : FALSE;
}

/* DVDConvertPathToEntrynum - Get entry number for path */
s32 DVDConvertPathToEntrynum(char* path) {
    DVD_LOG("DVDConvertPathToEntrynum(\"%s\")", path ? path : "(null)");

    if (!path) return -1;

    /* Look up in our table */
    for (int i = 0; file_entries[i].path != NULL; i++) {
        if (strcmp(path, file_entries[i].path) == 0) {
            DVD_LOG("  -> entry %d", file_entries[i].entry_num);
            return file_entries[i].entry_num;
        }
    }

    /* Check if file exists even if not in table */
    char pc_path[512];
    if (build_pc_path(path, pc_path, sizeof(pc_path)) >= 0) {
        /* File exists, generate a temporary entry number */
        static int next_dynamic_entry = 100;
        DVD_LOG("  -> dynamic entry %d", next_dynamic_entry);
        return next_dynamic_entry++;
    }

    DVD_LOG("  -> NOT FOUND (-1)");
    return -1;
}

/* DVDGetCurrentDiskID - Get disk ID */
DVDDiskID* DVDGetCurrentDiskID(void) {
    extern DVDDiskID DiskID;
    return &DiskID;
}

/* DVDGetTransferredSize */
s32 DVDGetTransferredSize(DVDFileInfo* fileInfo) {
    if (!fileInfo) return 0;
    return (s32)fileInfo->cb.transferredSize;
}

/* DVDGetCommandBlockStatus */
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block) {
    (void)block;
    return DVD_STATE_END;  /* Always ready on PC */
}

/* DVDGetDriveStatus */
s32 DVDGetDriveStatus(void) {
    return DVD_STATE_END;
}

/* Directory functions (stubbed for now) */
BOOL DVDOpenDir(char* dirName, DVDDir* dir) {
    DVD_LOG("DVDOpenDir(\"%s\") - STUB", dirName ? dirName : "(null)");
    if (dir) {
        dir->entryNum = 0;
        dir->location = 0;
        dir->next = 0;
    }
    return TRUE;
}

BOOL DVDReadDir(DVDDir* dir, DVDDirEntry* dirEntry) {
    (void)dir; (void)dirEntry;
    return FALSE;  /* No more entries */
}

BOOL DVDCloseDir(DVDDir* dir) {
    (void)dir;
    return TRUE;
}

BOOL DVDChangeDir(char* dirName) {
    DVD_LOG("DVDChangeDir(\"%s\") - STUB", dirName ? dirName : "(null)");
    return TRUE;
}

BOOL DVDGetCurrentDir(char* path, u32 maxLength) {
    if (path && maxLength > 1) {
        path[0] = '/';
        path[1] = '\0';
    }
    return TRUE;
}

/* Other stubs */
void DVDResume(void) { }
void DVDReset(void) { }
void DVDPause(void) { }

BOOL DVDSetAutoInvalidation(BOOL doAutoInval) {
    (void)doAutoInval;
    return TRUE;
}

void* DVDGetFSTLocation(void) {
    return NULL;
}

s32 DVDCancel(volatile DVDCommandBlock* block) {
    (void)block;
    return DVD_RESULT_GOOD;
}

BOOL DVDCancelAsync(DVDCommandBlock* block, DVDCBCallback callback) {
    if (callback) callback(DVD_RESULT_GOOD, block);
    return TRUE;
}

BOOL DVDCompareDiskID(DVDDiskID* id1, DVDDiskID* id2) {
    if (!id1 || !id2) return FALSE;
    return memcmp(id1, id2, sizeof(DVDDiskID)) == 0;
}

s32 DVDChangeDisk(DVDCommandBlock* block, DVDDiskID* id) {
    (void)block; (void)id;
    return DVD_RESULT_GOOD;
}

BOOL DVDChangeDiskAsync(DVDCommandBlock* block, DVDDiskID* id, DVDCBCallback callback) {
    (void)block; (void)id;
    if (callback) callback(DVD_RESULT_GOOD, block);
    return TRUE;
}

s32 DVDSeekPrio(DVDFileInfo* fileInfo, s32 offset, s32 prio) {
    (void)prio;
    if (!fileInfo) return DVD_RESULT_FATAL_ERROR;

    int handle = (int)fileInfo->startAddr;
    if (handle < 0 || handle >= MAX_DVD_FILES || !dvd_files[handle].in_use) {
        return DVD_RESULT_FATAL_ERROR;
    }

    if (fseek(dvd_files[handle].fp, offset, SEEK_SET) != 0) {
        return DVD_RESULT_FATAL_ERROR;
    }

    return DVD_RESULT_GOOD;
}

BOOL DVDSeekAsyncPrio(DVDFileInfo* fileInfo, s32 offset, DVDCallback callback, s32 prio) {
    s32 result = DVDSeekPrio(fileInfo, offset, prio);
    if (callback) callback(result, fileInfo);
    return (result == DVD_RESULT_GOOD);
}

/* Streaming stubs */
BOOL DVDPrepareStreamAsync(DVDFileInfo* fileInfo, u32 length, u32 offset, DVDCallback callback) {
    (void)fileInfo; (void)length; (void)offset;
    if (callback) callback(DVD_RESULT_GOOD, fileInfo);
    return TRUE;
}

s32 DVDCancelStream(DVDCommandBlock* block) {
    (void)block;
    return DVD_RESULT_GOOD;
}

DVDLowCallback DVDLowClearCallback(void) {
    return NULL;
}

BOOL DVDFastOpenDir(s32 entryNum, DVDDir* dir) {
    (void)entryNum;
    if (dir) {
        dir->entryNum = 0;
        dir->location = 0;
        dir->next = 0;
    }
    return TRUE;
}

BOOL DVDCancelAllAsync(DVDCBCallback callback) {
    if (callback) callback(DVD_RESULT_GOOD, NULL);
    return TRUE;
}

s32 DVDCancelAll(void) {
    return DVD_RESULT_GOOD;
}

void DVDDumpWaitingQueue(void) {
    DVD_LOG("DVDDumpWaitingQueue() - no queue on PC");
}

/* Internal init function */
void __DVDFSInit(void) {
    DVDInit();
}
