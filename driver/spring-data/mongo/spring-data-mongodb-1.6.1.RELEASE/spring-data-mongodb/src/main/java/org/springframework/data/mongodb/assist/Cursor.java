package org.springframework.data.mongodb.assist;

import java.io.Closeable;
import java.util.Iterator;

/**
 * Created by tanzhaobo on 2017/9/7.
 */
public interface Cursor extends Iterator<DBObject>, Closeable {
    long getCursorId();

    ServerAddress getServerAddress();

    void close();
}
