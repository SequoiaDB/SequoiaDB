package com.sequoiadb.transaction.metadataserial;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.rename.RenameUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-22238:事务读操作的过程中修改cs名
 * @author wuyan
 * @Date 2020.06.04
 * @version 1.0
 */
public class Transaction22238A extends SdbTestBase {

    private String csName = "renameCS_22238A";
    private String newCSName = "renameCS_22238_newA";
    private String clName = "rename_22238A";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private int recordNum = 10000;
    private ArrayList<BSONObject> actQueryRecsList = new ArrayList<>();
    private boolean isStartQuery = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        cs = sdb.createCollectionSpace(csName);
        DBCollection cl = cs.createCollection(clName);
        RenameUtil.insertData(cl, recordNum);
    }

    @Test
    public void test() {
        RenameCSThread renameCSThread = new RenameCSThread();
        TransactionThread transThread = new TransactionThread();

        renameCSThread.start();
        transThread.start();

        Assert.assertTrue(renameCSThread.isSuccess(), renameCSThread.getErrorMsg());
        Assert.assertTrue(transThread.isSuccess(), transThread.getErrorMsg());

        try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "");) {
            RenameUtil.checkRenameCSResult(db, csName, newCSName, 1);
            Assert.assertEquals(actQueryRecsList.size(), recordNum, "check record count");
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS(sdb, csName);
            CommLib.clearCS(sdb, newCSName);
        } finally {
            if (sdb != null) {
                sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
                // 等待开始执行查询再rename,最长等待2分钟
                int eachSleepTime = 2;
                int maxWaitTime = 120000;
                int alreadyWaitTime = 0;
                do {
                    try {
                        Thread.sleep(eachSleepTime);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    alreadyWaitTime += eachSleepTime;
                    if (alreadyWaitTime > maxWaitTime) {
                        Assert.fail("---not query started in maxWaitTime ! waitTime is" + alreadyWaitTime);
                    }
                } while (!isStartQuery);
                db.renameCollectionSpace(csName, newCSName);
            }
        }
    }

    private class TransactionThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
                DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
                TransUtils.beginTransaction(db);
                DBCursor cursor = cl.query();
                while (cursor.hasNext()) {
                    isStartQuery = true;
                    BSONObject record = cursor.getNext();
                    actQueryRecsList.add(record);
                }
                cursor.close();
                db.commit();
            }
        }
    }

}
