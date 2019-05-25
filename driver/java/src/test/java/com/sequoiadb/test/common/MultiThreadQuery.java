package com.sequoiadb.test.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class MultiThreadQuery implements Runnable {
    Sequoiadb sdb;
    CollectionSpace cs;
    DBCollection cl;
    DBCursor cursor;

    public MultiThreadQuery() {
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
        System.out.println("Query�߳�===" + Thread.currentThread().getId() + "ִ�п�ʼ");
        for (int j = 0; j < 10; j++) {
            BSONObject obj = new BasicBSONObject();
            obj.put("ThreadID", Thread.currentThread().getId() - 1);
            obj.put("NO", (Thread.currentThread().getId() - 1) + "_" + String.valueOf(j));
            cursor = cl.query(obj, null, null, null);
            int size = 0;
            while (cursor.hasNext()) {
                cursor.getNext();
                size++;
            }
            System.out.println("size=" + size);
        }
        System.out.println("Query�߳�===" + Thread.currentThread().getId() + "ִ�н���");
    }
}
