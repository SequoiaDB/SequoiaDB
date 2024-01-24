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
 * @Description seqDB-22238:事务读操作的过程中修改cL名
 * @author wuyan
 * @Date 2020.06.04
 * @version 1.0
 */
public class Transaction22238B extends SdbTestBase {

    private String clName = "renameCL_22238B";
    private String newCLName = "renameCL_22238_newB";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private int recordNum = 10000;
    private ArrayList<BSONObject> actQueryRecsList = new ArrayList<>();
    private boolean isStartQuery = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        cs = sdb.getCollectionSpace(SdbTestBase.csName);
        DBCollection cl = cs.createCollection(clName);
        RenameUtil.insertData(cl, recordNum);
    }

    @Test
    public void test() {
        TransactionThread transThread = new TransactionThread();
        RenameCLThread renameCLThread = new RenameCLThread();

        transThread.start();
        renameCLThread.start();

        Assert.assertTrue(renameCLThread.isSuccess(), renameCLThread.getErrorMsg());
        Assert.assertTrue(transThread.isSuccess(), transThread.getErrorMsg());

        try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "");) {
            RenameUtil.checkRenameCLResult(sdb, SdbTestBase.csName, clName, newCLName);
            Assert.assertEquals(actQueryRecsList.size(), recordNum, "check record count");
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL(sdb, SdbTestBase.csName, clName);
            CommLib.clearCL(sdb, SdbTestBase.csName, newCLName);
        } finally {
            if (sdb != null) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
                CollectionSpace cs = db.getCollectionSpace(SdbTestBase.csName);
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
                cs.renameCollection(clName, newCLName);
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
