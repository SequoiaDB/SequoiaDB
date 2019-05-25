package org.springframework.data.sequoiadb.assist;

import org.bson.BSONObject;

import java.io.Closeable;
import java.util.Iterator;

/**
 * Created by tanzhaobo on 2017/9/7.
 */
public interface Cursor extends Iterator<BSONObject>, Closeable {
    long getCursorId();

    ServerAddress getServerAddress();

    void close();
}
