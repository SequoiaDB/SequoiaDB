package com.sequoiadb.test.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.ArrayList;
import java.util.List;

public class MultiThreadInsert implements Runnable {
    Sequoiadb sdb;
    CollectionSpace cs;
    DBCollection cl;
    int num = 10;

    public MultiThreadInsert() {
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
        //System.out.println("Insert�߳�==="+Thread.currentThread().getId()+"ִ�п�ʼ");
        List<BSONObject> list = null;
        list = new ArrayList<BSONObject>();
        for (int j = 0; j < num; j++) {
            BSONObject obj = new BasicBSONObject();
            obj.put("ThreadID", Thread.currentThread().getId());
            obj.put("NO", Thread.currentThread().getId() + "_" + String.valueOf(j));
            list.add(obj);
        }
        cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
        //System.out.println("Insert�߳�==="+Thread.currentThread().getId()+"ִ�н���");
    }
}
