package org.springframework.data.sequoiadb.assist;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import java.net.UnknownHostException;
import java.util.Set;

/**
 * Created by tanzhaobo on 2017/10/10.
 */
public class Helper {

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
