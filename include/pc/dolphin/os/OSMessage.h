/**
 * @file OSMessage.h
 * @brief PC override for dolphin/os/OSMessage.h
 *
 * Routes to our unified os_compat.h which has all OS types
 */
#ifndef PC_DOLPHIN_OS_OSMESSAGE_H
#define PC_DOLPHIN_OS_OSMESSAGE_H

/* All OS types and message queue functions are in os_compat.h */
#include "pc/os_compat.h"

/* GameCube-style aliases if needed */
typedef OSMesgQueue OSMessageQueue;
typedef OSMesg OSMessage;

/* GameCube message flags (map to N64 style) */
#define OS_MESSAGE_NOBLOCK OS_MESG_NOBLOCK
#define OS_MESSAGE_BLOCK   OS_MESG_BLOCK

/* GameCube-style function aliases */
#define OSInitMessageQueue(queue, msgArray, msgCount) \
    osCreateMesgQueue((queue), (msgArray), (msgCount))
#define OSSendMessage(queue, msg, flags) \
    osSendMesg((queue), (msg), (flags))
#define OSJamMessage(queue, msg, flags) \
    osSendMesg((queue), (msg), (flags))  /* Simplified - jam at front not implemented */
#define OSReceiveMessage(queue, msgPtr, flags) \
    osRecvMesg((queue), (msgPtr), (flags))

#endif /* PC_DOLPHIN_OS_OSMESSAGE_H */
