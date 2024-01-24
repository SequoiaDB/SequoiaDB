package org.springframework.data.mongodb.assist;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import java.net.UnknownHostException;
import java.util.Set;

/**
 * Created by tanzhaobo on 2017/10/10.
 */
public class Helper {

    public static BasicDBObject BasicBSONObjectToBasicDBObject(BasicBSONObject obj) {
        if (obj == null) {
            return null;
        }
        BasicDBObject returnObj = new BasicDBObject();
        Set<String> keys = obj.keySet();
        for (String key : keys) {
            Object valueObj = obj.get(key);
            if (valueObj instanceof BasicBSONList) {
                returnObj.put(key, BasicBSONListToBasicDBList((BasicBSONList)valueObj));
            } else if (valueObj instanceof  BasicBSONObject) {
                returnObj.put(key, BasicBSONObjectToBasicDBObject((BasicBSONObject)valueObj));
            } else {
                returnObj.put(key, valueObj);
            }
        }
        return returnObj;
    }

    public static BasicDBList BasicBSONListToBasicDBList(BasicBSONList list) {
        if (list == null) {
            return null;
        }
        BasicDBList returnList = new BasicDBList();
        Set<String> keys = list.keySet();
        for(String key : keys) {
            Object valueObj = list.get(key);
            if (valueObj instanceof BasicBSONList) {
                returnList.put(key, BasicBSONListToBasicDBList((BasicBSONList)valueObj));
            } else if (valueObj instanceof  BasicBSONObject) {
                returnList.put(key, BasicBSONObjectToBasicDBObject((BasicBSONObject)valueObj));
            } else {
                returnList.put(key, valueObj);
            }
        }
        return returnList;
    }

    public static WriteResult getDefaultWriteResult(com.sequoiadb.base.Sequoiadb db) {
        ServerAddress address;
        try {
            address = new ServerAddress(db.getServerAddress().getHost(), db.getServerAddress().getPort());
        } catch (UnknownHostException e) {
            address = null;
        }
        return new WriteResult(address);
    }
}
