package com.sequoiadb.test.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class MultiThreadCreateGetDropCS implements Runnable {
    Sequoiadb sdb;
    CollectionSpace cs;
    DBCollection cl;

    public MultiThreadCreateGetDropCS() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @Override
    public void run() {
        try {
            sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            System.out.println(this.hashCode() + " " + e.getMessage());
        }
        try {
            sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            System.out.println(this.hashCode() + " " + e.getMessage());
        }
        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            System.out.println(this.hashCode() + " " + e.getMessage());
        }
    }
}