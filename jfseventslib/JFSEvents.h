//
//  JFSEvents.h
//  jfseventslib
//
//  Created by Sigurður Jón on 2023-04-15.
//

#ifndef JFSEvents_h
#define JFSEvents_h

#include <stdio.h>
#include <jni.h>

struct EventItem {
    struct EventItem* nextItem;
    int64_t id;
    char* path;
    int32_t flags;
};

struct JFSEventsHandle {
    jobject javaObject;
    volatile bool started;
    volatile bool stopped;
    CFMutableArrayRef monitoredPaths;
    CFMutableArrayRef excludedPaths;
    FSEventStreamRef stream;
    pthread_mutex_t lock;
    bool reading;
    struct EventItem* firstItem;
    struct EventItem* lastItem;
};

CFStringRef to_cfstring(JNIEnv *env, jstring path);
void fs_callback(ConstFSEventStreamRef streamRef,
                 void *clientCallBackInfo,
                 size_t numEvents,
                 void *eventPaths,
                 const FSEventStreamEventFlags eventFlags[],
                 const FSEventStreamEventId eventIds[]);

struct EventItem* readEventItem(struct JFSEventsHandle* handle);
void createAndAppendEventItem(struct JFSEventsHandle* handle, long eventId, char* path, int eventFlags);
void appendEventItem(struct JFSEventsHandle* handle, struct EventItem* item);
void freeHandle(struct JFSEventsHandle* handle);
long monotonicTime(void);

#endif /* JFSEvents_h */
