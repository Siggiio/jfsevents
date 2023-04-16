package io.siggi.jfsevents;

import java.util.Objects;

public final class FSEvent {
    private final long id;
    private final String path;
    private final int flags;

    FSEvent(long id, String path, int flags) {
        if (path == null) throw new NullPointerException();
        this.id = id;
        this.path = path;
        this.flags = flags;
    }

    public long getId() {
        return id;
    }

    public String getPath() {
        return path;
    }

    public int getFlags() {
        return flags;
    }

    @Override
    public String toString() {
        return "FSEvent{id=" + id + ",flags=0x" + Integer.toString(flags, 16) + ",path=" + path + "}";
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        FSEvent fsEvent = (FSEvent) o;
        return id == fsEvent.id && flags == fsEvent.flags && path.equals(fsEvent.path);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id, path, flags);
    }
}
