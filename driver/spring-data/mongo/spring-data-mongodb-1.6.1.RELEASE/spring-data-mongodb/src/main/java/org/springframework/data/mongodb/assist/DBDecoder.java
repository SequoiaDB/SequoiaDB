package org.springframework.data.mongodb.assist;

import org.bson.BSONDecoder;

import java.io.IOException;
import java.io.InputStream;

/**
 * Created by tanzhaobo on 2017/9/7.
 */
public interface DBDecoder extends BSONDecoder {
    public DBCallback getDBCallback(DBCollection collection);

    public DBObject decode( byte[] b, DBCollection collection );

    public DBObject decode(InputStream in, DBCollection collection ) throws IOException;
}
