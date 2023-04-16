//
//  JFSEvents.c
//  jfseventslib
//
//  Created by Sigurður Jón on 2023-04-15.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <CoreServices/CoreServices.h>
#include <sys/stat.h>
#include "JFSEvents.h"
#include "io_siggi_jfsevents_JFSEvents.h"

static JavaVM *jvm;

static jclass jfsEventsClass;
static jmethodID checkForInterruptionMethod;
static jfieldID handleField;

static jclass eventClass;
static jfieldID idField;
static jfieldID pathField;
static jfieldID flagsField;

static jclass uuidClass;
static jmethodID uuidConstructor;

static jclass illegalStateExceptionClass;

CFStringRef to_cfstring(JNIEnv *env, jstring path) {
    const char *cpath = (*env)->GetStringUTFChars(env, path, NULL);
    CFStringRef stringRef = CFStringCreateWithCString(kCFAllocatorDefault, cpath, kCFStringEncodingUTF8);
    (*env)->ReleaseStringUTFChars(env, path, cpath);
    return stringRef;
}

void fs_callback(ConstFSEventStreamRef streamRef,
                 void *callbackRef,
                 size_t numEvents,
                 void *eventPaths,
                 const FSEventStreamEventFlags eventFlags[],
                 const FSEventStreamEventId eventIds[]) {
    struct JFSEventsHandle* handle = (struct JFSEventsHandle*) callbackRef;
    char **paths = eventPaths;
    pthread_mutex_lock(&(handle->lock));
    for (int i = 0; i < numEvents; i++) {
        createAndAppendEventItem(handle, eventIds[i], paths[i], eventFlags[i]);
    }
    pthread_mutex_unlock(&(handle->lock));
}

struct EventItem* readEventItem(struct JFSEventsHandle* handle) {
    struct EventItem* item = handle->firstItem;
    if (item == NULL) return NULL;
    if (item->nextItem == NULL) {
        handle->firstItem = NULL;
        handle->lastItem = NULL;
    } else {
        handle->firstItem = item->nextItem;
    }
    return item;
}

void createAndAppendEventItem(struct JFSEventsHandle* handle, long eventId, char* path, int eventFlags) {
    size_t length = strlen(path);
    char* copyOfPath = malloc(length + 1);
    strcpy(copyOfPath, path);
    struct EventItem* item = malloc(sizeof(struct EventItem));
    item->id = eventId;
    item->path = copyOfPath;
    item->flags = eventFlags;
    appendEventItem(handle, item);
}

void appendEventItem(struct JFSEventsHandle* handle, struct EventItem* item) {
    if (item == NULL) return;
    if (handle->firstItem == NULL || handle->lastItem == NULL) {
        handle->firstItem = item;
        handle->lastItem = item;
    } else {
        handle->lastItem->nextItem = item;
        handle->lastItem = item;
    }
}

void freeHandle(struct JFSEventsHandle* handle) {
    while (handle->firstItem != NULL) {
        struct EventItem* item = handle->firstItem;
        handle->firstItem = item->nextItem;
        free(item);
    }
    pthread_mutex_destroy(&(handle->lock));
    free(handle);
}

long monotonicTime(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
}

bool hasTimeExpired(long start, long end, long now) {
    if (end < start) {
        return now < start && now >= end;
    } else {
        return now >= end;
    }
}

JNIEXPORT jlong JNICALL Java_io_siggi_jfsevents_JFSEvents_allocate
  (JNIEnv *env, jclass clazz, jobject javaObject) {
    struct JFSEventsHandle* handle = malloc(sizeof(struct JFSEventsHandle));
    pthread_mutex_init(&(handle->lock), NULL);
    handle->javaObject = (*env)->NewGlobalRef(env, javaObject);
    handle->started = false;
    handle->monitored_paths = CFArrayCreateMutable(NULL, 0, NULL);
    return (long) handle;
}

JNIEXPORT void JNICALL Java_io_siggi_jfsevents_JFSEvents_deallocate
  (JNIEnv *env, jclass clazz, jlong longHandle) {
    struct JFSEventsHandle* handle = (struct JFSEventsHandle*) longHandle;
    pthread_mutex_lock(&(handle->lock));
    if (handle->started) {
        FSEventStreamStop(handle->stream);
        FSEventStreamInvalidate(handle->stream);
        FSEventStreamRelease(handle->stream);
        handle->stopped = true;
    }
    for (int i = 0; i < CFArrayGetCount(handle->monitored_paths); i++) {
        CFStringRef str = CFArrayGetValueAtIndex(handle->monitored_paths, i);
        CFRelease(str);
    }
    CFRelease(handle->monitored_paths);
    (*env)->SetLongField(env, handle->javaObject, handleField, 0);
    (*env)->DeleteGlobalRef(env, handle->javaObject);
    bool canFree = !handle->reading;
    pthread_mutex_unlock(&(handle->lock));
    if (canFree) {
        freeHandle(handle);
    }
}

JNIEXPORT void JNICALL Java_io_siggi_jfsevents_JFSEvents_addPath
  (JNIEnv *env, jclass clazz, jlong longHandle, jstring path) {
    struct JFSEventsHandle* handle = (struct JFSEventsHandle*) longHandle;
    if (handle->started) {
        (*env)->ThrowNew(env, illegalStateExceptionClass, "JFSEvents already started");
        return;
    }
    CFArrayAppendValue(handle->monitored_paths, to_cfstring(env, path));
}

JNIEXPORT void JNICALL Java_io_siggi_jfsevents_JFSEvents_start
  (JNIEnv *env, jclass clazz, jlong longHandle, jlong device, jlong sinceEvent, jdouble latency, jint flags) {
    struct JFSEventsHandle* handle = (struct JFSEventsHandle*) longHandle;
    if (handle->started) {
        (*env)->ThrowNew(env, illegalStateExceptionClass, "JFSEvents already started");
        return;
    }
    handle->started = true;
    FSEventStreamEventId sinceEventId;
    if (sinceEvent == -1L) {
        sinceEventId = kFSEventStreamEventIdSinceNow;
    } else {
        sinceEventId = sinceEvent;
    }
    FSEventStreamContext streamContext = {
        0, handle, NULL, NULL, NULL
    };
    if (device == 0) {
        handle->stream = FSEventStreamCreate(NULL,
                                             &fs_callback,
                                             &streamContext,
                                             handle->monitored_paths,
                                             sinceEventId,
                                             latency,
                                             flags
                                             );
    } else {
        handle->stream = FSEventStreamCreateRelativeToDevice(NULL,
                                                             &fs_callback,
                                                             &streamContext,
                                                             (dev_t) device,
                                                             handle->monitored_paths,
                                                             sinceEventId,
                                                             latency,
                                                             flags
                                                             );
    }
    FSEventStreamSetDispatchQueue(handle->stream, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
    FSEventStreamStart(handle->stream);
}

JNIEXPORT jobject JNICALL Java_io_siggi_jfsevents_JFSEvents_readEvent
  (JNIEnv *env, jclass clazz, jlong longHandle, jlong timeout) {
    struct JFSEventsHandle* handle = (struct JFSEventsHandle*) longHandle;
    if (!handle->started) {
        (*env)->ThrowNew(env, illegalStateExceptionClass, "JFSEvents was not started yet.");
        return NULL;
    }
    long startTime = monotonicTime();
    long endTime = startTime + (timeout);
    bool didSetReading = false;
    bool shouldFree = false;
    struct EventItem* item = NULL;
    do {
        pthread_mutex_lock(&(handle->lock));
        if (handle->stopped) {
            if (handle->reading) {
                shouldFree = true;
                pthread_mutex_unlock(&(handle->lock));
                break;
            }
        }
        if (!didSetReading) {
            didSetReading = true;
            if (handle->reading) {
                (*env)->ThrowNew(env, illegalStateExceptionClass, "Simultaneous read on JFSEvents is not allowed.");
                pthread_mutex_unlock(&(handle->lock));
                break;
            }
            handle->reading = true;
        }
        item = readEventItem(handle);
        if (item != NULL) {
            handle->reading = false;
        } else {
            (*env)->CallStaticVoidMethod(env, jfsEventsClass, checkForInterruptionMethod);
            if ((*env)->ExceptionCheck(env) || (timeout >= 0L && hasTimeExpired(startTime, endTime, monotonicTime()))) {
                handle->reading = false;
                pthread_mutex_unlock(&(handle->lock));
                break;
            }
        }
        pthread_mutex_unlock(&(handle->lock));
        if (item == NULL) {
            usleep(1000);
        }
    } while (item == NULL);
    if (shouldFree) {
        freeHandle(handle);
    }
    if (item == NULL) return NULL;
    jobject event = (*env)->AllocObject(env, eventClass);
    (*env)->SetLongField(env, event, idField, item->id);
    jstring pathJString = (*env)->NewStringUTF(env, item->path);
    (*env)->SetObjectField(env, event, pathField, pathJString);
    (*env)->SetIntField(env, event, flagsField, item->flags);
    (*env)->DeleteLocalRef(env, pathJString);
    free(item->path);
    free(item);
    return event;
}

JNIEXPORT jlong JNICALL Java_io_siggi_jfsevents_JFSEvents_getDeviceId
  (JNIEnv *env, jclass clazz, jstring javaPath) {
    const char *path = (*env)->GetStringUTFChars(env, javaPath, 0);
    struct stat pathInfo;
    stat(path, &pathInfo);
    (*env)->ReleaseStringUTFChars(env, javaPath, path);
    return pathInfo.st_dev;
}

JNIEXPORT jobject JNICALL Java_io_siggi_jfsevents_JFSEvents_getUuidForDevice
  (JNIEnv *env, jclass clazz, jlong device) {
    CFUUIDRef uuidRef = FSEventsCopyUUIDForDevice((dev_t) device);
    if (uuidRef == NULL) return NULL;
    CFUUIDBytes uuidBytesPtr = CFUUIDGetUUIDBytes(uuidRef);
    CFUUIDBytes* uuidBytes = (CFUUIDBytes*) &uuidBytesPtr;
    int64_t mostSignificant =
        (((int64_t) uuidBytes->byte0) << 56) | (((int64_t) uuidBytes->byte1) << 48)
      | (((int64_t) uuidBytes->byte2) << 40) | (((int64_t) uuidBytes->byte3) << 32)
      | (((int64_t) uuidBytes->byte4) << 24) | (((int64_t) uuidBytes->byte5) << 16)
      | (((int64_t) uuidBytes->byte6) << 8) | ((int64_t) uuidBytes->byte7);
    int64_t leastSignificant =
         (((int64_t) uuidBytes->byte8) << 56) | (((int64_t) uuidBytes->byte9) << 48)
      | (((int64_t) uuidBytes->byte10) << 40) | (((int64_t) uuidBytes->byte11) << 32)
      | (((int64_t) uuidBytes->byte12) << 24) | (((int64_t) uuidBytes->byte13) << 16)
      | (((int64_t) uuidBytes->byte14) << 8) | ((int64_t) uuidBytes->byte15);
    jobject uuidObject = (*env)->NewObject(env, uuidClass, uuidConstructor, mostSignificant, leastSignificant);
    CFRelease(uuidRef);
    return uuidObject;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jvm = vm;
    JNIEnv *env;
    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_8);

    jclass localJFSEventsClass = (*env)->FindClass(env, "io/siggi/jfsevents/JFSEvents");
    jfsEventsClass = (*env)->NewGlobalRef(env, localJFSEventsClass);
    (*env)->DeleteLocalRef(env, localJFSEventsClass);
    checkForInterruptionMethod = (*env)->GetStaticMethodID(env, jfsEventsClass, "checkForInterruption", "()V");
    handleField = (*env)->GetFieldID(env, jfsEventsClass, "handle", "J");

    jclass localEventClass = (*env)->FindClass(env, "io/siggi/jfsevents/FSEvent");
    eventClass = (*env)->NewGlobalRef(env, localEventClass);
    (*env)->DeleteLocalRef(env, localEventClass);
    idField = (*env)->GetFieldID(env, eventClass, "id", "J");
    pathField = (*env)->GetFieldID(env, eventClass, "path", "Ljava/lang/String;");
    flagsField = (*env)->GetFieldID(env, eventClass, "flags", "I");
    
    jclass localUuidClass = (*env)->FindClass(env, "java/util/UUID");
    uuidClass = (*env)->NewGlobalRef(env, localUuidClass);
    (*env)->DeleteLocalRef(env, localUuidClass);
    uuidConstructor = (*env)->GetMethodID(env, uuidClass, "<init>", "(JJ)V");

    jclass localIllegalStateExceptionClass = (*env)->FindClass(env, "java/lang/IllegalStateException");
    illegalStateExceptionClass = (*env)->NewGlobalRef(env, localIllegalStateExceptionClass);
    (*env)->DeleteLocalRef(env, localIllegalStateExceptionClass);

    return JNI_VERSION_1_8;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    (*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_8);
    (*env)->DeleteGlobalRef(env, jfsEventsClass);
    (*env)->DeleteGlobalRef(env, eventClass);
    (*env)->DeleteGlobalRef(env, uuidClass);
    (*env)->DeleteGlobalRef(env, illegalStateExceptionClass);
}


