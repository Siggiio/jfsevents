package io.siggi.jfsevents;

public class FSEventConstants {
    private FSEventConstants() {
    }

    public static final int kFSEventStreamCreateFlagNone = 0x00;
    public static final int kFSEventStreamCreateFlagUseCFTypes = 0x01;
    public static final int kFSEventStreamCreateFlagNoDefer = 0x02;
    public static final int kFSEventStreamCreateFlagWatchRoot = 0x04;
    public static final int kFSEventStreamCreateFlagIgnoreSelf = 0x08; // since 10.6
    public static final int kFSEventStreamCreateFlagFileEvents = 0x10; // since 10.7
    public static final int kFSEventStreamCreateFlagMarkSelf = 0x20; // since 10.9
    public static final int kFSEventStreamCreateFlagUseExtendedData = 0x40; // since 10.13
    public static final int kFSEventStreamCreateFlagFullHistory = 0x80; // since 10.15

    public static final int kFSEventStreamEventFlagNone = 0x00000000;
    public static final int kFSEventStreamEventFlagMustScanSubDirs = 0x00000001;
    public static final int kFSEventStreamEventFlagUserDropped = 0x00000002;
    public static final int kFSEventStreamEventFlagKernelDropped = 0x00000004;
    public static final int kFSEventStreamEventFlagEventIdsWrapped = 0x00000008;
    public static final int kFSEventStreamEventFlagHistoryDone = 0x00000010;
    public static final int kFSEventStreamEventFlagRootChanged = 0x00000020;
    public static final int kFSEventStreamEventFlagMount  = 0x00000040;
    public static final int kFSEventStreamEventFlagUnmount = 0x00000080;
    public static final int kFSEventStreamEventFlagItemCreated = 0x00000100; // since 10.7
    public static final int kFSEventStreamEventFlagItemRemoved = 0x00000200; // since 10.7
    public static final int kFSEventStreamEventFlagItemInodeMetaMod = 0x00000400; // since 10.7
    public static final int kFSEventStreamEventFlagItemRenamed = 0x00000800; // since 10.7
    public static final int kFSEventStreamEventFlagItemModified = 0x00001000; // since 10.7
    public static final int kFSEventStreamEventFlagItemFinderInfoMod = 0x00002000; // since 10.7
    public static final int kFSEventStreamEventFlagItemChangeOwner = 0x00004000; // since 10.7
    public static final int kFSEventStreamEventFlagItemXattrMod = 0x00008000; // since 10.7
    public static final int kFSEventStreamEventFlagItemIsFile = 0x00010000; // since 10.7
    public static final int kFSEventStreamEventFlagItemIsDir = 0x00020000; // since 10.7
    public static final int kFSEventStreamEventFlagItemIsSymlink = 0x00040000; // since 10.7
    public static final int kFSEventStreamEventFlagOwnEvent = 0x00080000; // since 10.9
    public static final int kFSEventStreamEventFlagItemIsHardlink = 0x00100000; // since 10.10
    public static final int kFSEventStreamEventFlagItemIsLastHardlink = 0x00200000; // since 10.10
    public static final int kFSEventStreamEventFlagItemCloned = 0x00400000; // since 10.13

}
