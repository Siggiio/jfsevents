package io.siggi.jfsevents;

import java.io.Closeable;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.UUID;

public class JFSEvents implements Closeable {
    // <editor-fold desc="JNI setup" defaultstate="collapsed">
    static {
        File tmpFile = null;
        try {
            try (InputStream in = JFSEvents.class.getResourceAsStream("/libjfseventslib.dylib")) {
                tmpFile = File.createTempFile("libjfseventslib", ".dylib");
                try (FileOutputStream out = new FileOutputStream(tmpFile)) {
                    byte[] b = new byte[4096];
                    int c;
                    while ((c = in.read(b, 0, b.length)) != -1) {
                        out.write(b, 0, c);
                    }
                }
            }
            System.load(tmpFile.getAbsolutePath());
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (tmpFile != null)
                tmpFile.delete();
        }
    }
    // </editor-fold>

    private final long handle;
    private boolean closed = false;

    public JFSEvents() {
        handle = allocate(this);
    }

    public void close() {
        if (closed) return;
        closed = true;
        deallocate(handle);
    }

    public void addPath(String path) {
        if (closed) throw new IllegalStateException("JFSEvents already closed");
        addPath(handle, path);
    }

    public void start(long sinceEvent, double latency, int flags) {
        start(0L, sinceEvent, latency, flags);
    }

    public void start(long device, long sinceEvent, double latency, int flags) {
        if (closed) throw new IllegalStateException("JFSEvents already closed");
        start(handle, device, sinceEvent, latency, flags);
    }

    public FSEvent readEvent() throws InterruptedException {
        return readEvent(-1L);
    }

    public FSEvent readEvent(long timeout) throws InterruptedException {
        if (closed) throw new IllegalStateException("JFSEvents already closed");
        return readEvent(handle, timeout);
    }

    private static native long allocate(JFSEvents javaObject);

    private static native void deallocate(long handle);

    private static native void addPath(long handle, String path);

    private static native void start(long handle, long device, long sinceEvent, double latency, int flags);

    private static native FSEvent readEvent(long handle, long timeout) throws InterruptedException;

    public static native long getDeviceId(String path);

    public static native UUID getUuidForDevice(long device);

    private static void checkForInterruption() throws InterruptedException {
        if (Thread.interrupted()) throw new InterruptedException();
    }
}
