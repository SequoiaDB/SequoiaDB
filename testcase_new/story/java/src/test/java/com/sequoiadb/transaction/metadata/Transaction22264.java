package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description seqDB-22264:事务读操作的过程中修改cl属性
 * @author wuyan
 * @Date 2020.06.04
 * @version 1.0
 */
public class Transaction22264 extends SdbTestBase {

    private String clName = "alterCL_22264";
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
        BSONObject option = new BasicBSONObject();
        option.put("CompressionType", "snappy");
        option.put("ShardingKey", new BasicBSONObject("a", 1));

        AlterCLThread alterCLThread = new AlterCLThread(option);
        TransactionThread transThread = new TransactionThread();

        alterCLThread.start();
        transThread.start();

        Assert.assertTrue(alterCLThread.isSuccess(), alterCLThread.getErrorMsg());
        Assert.assertTrue(transThread.isSuccess(), transThread.getErrorMsg());

        checkAlterReslut(option);
        try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "");) {
            Assert.assertEquals(actQueryRecsList.size(), recordNum, "check record count");
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL(sdb, SdbTestBase.csName, clName);
        } finally {
            if (sdb != null) {
                sdb.close();
            }
        }
    }

    private class AlterCLThread extends SdbThreadBase {
        private BSONObject option;

        public AlterCLThread(BSONObject option) {
            this.option = option;
        }

        @Override
        public void exec() throws Exception {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
                DBCollection dbcl = db.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
                // 等待开始执行查询再alter,最长等待2分钟
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
                dbcl.alterCollection(option);
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

    private void checkAlterReslut(BSONObject expected) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        matcher.put("Name", SdbTestBase.csName + "." + clName);
        try (DBCursor cur = sdb.getSnapshot(8, matcher, null, null)) {
            Assert.assertNotNull(cur.getNext());
            actual = cur.getCurrent();
            Assert.assertEquals(actual.get("ShardingKey").toString(), expected.get("ShardingKey").toString());
            Assert.assertEquals(actual.get("CompressionTypeDesc").toString(),
                    expected.get("CompressionType").toString());
        }
    }

}
