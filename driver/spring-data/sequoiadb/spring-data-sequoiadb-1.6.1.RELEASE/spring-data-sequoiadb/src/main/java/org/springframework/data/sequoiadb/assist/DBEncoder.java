package org.springframework.data.sequoiadb.assist;

/**
 * Created by tanzhaobo on 2017/9/7.
 */
import org.bson.BSONObject;
import org.bson.io.OutputBuffer;

public interface DBEncoder {
    public int writeObject( OutputBuffer buf, BSONObject o );
}