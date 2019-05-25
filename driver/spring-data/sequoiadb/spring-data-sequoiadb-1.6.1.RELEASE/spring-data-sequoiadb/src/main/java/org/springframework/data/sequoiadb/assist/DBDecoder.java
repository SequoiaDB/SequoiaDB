package org.springframework.data.sequoiadb.assist;

import org.bson.BSONDecoder;
import org.bson.BSONObject;

import java.io.IOException;
import java.io.InputStream;

/**
 * Created by tanzhaobo on 2017/9/7.
 */
public interface DBDecoder extends BSONDecoder {
    public DBCallback getDBCallback(DBCollection collection);

    public BSONObject decode(byte[] b, DBCollection collection );

    public BSONObject decode(InputStream in, DBCollection collection ) throws IOException;
}
