package com.sequoiadb.test.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class MultiThreadUpdate implements Runnable {
    Sequoiadb sdb;
    CollectionSpace cs;
    DBCollection cl;
    DBCursor cursor;

    public MultiThreadUpdate() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            cs = sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
            if (cs.isCollectionExist(Constants.TEST_CL_NAME_1))
                cl = cs.getCollection(Constants.TEST_CL_NAME_1);
            else
                cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
            cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        }
    }

    @Override
    public void run() {
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        modifier.put("$set", m);
        String field = "field" + Thread.currentThread().getId();
        m.put(field, Thread.currentThread().getName());

        cl.update(null, modifier, null);
    }
}