package org.springframework.data.mongodb.assist;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import java.util.Map;

/**
 * Created by tanzhaobo on 2017/9/1.
 */
public class BasicDBObject extends BasicBSONObject implements DBObject{
    BasicDBObject(BasicBSONObject o) {
        super(o);
    }

    public BasicDBObject() {
        super();
    }

    public BasicDBObject(int size) {
        super(size);
    }

    public BasicDBObject(String key, Object value) {
        super(key, value);
    }

    public BasicDBObject(Map m) {
        super(m);
    }
}
