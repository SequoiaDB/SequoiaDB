package com.sequoiadb.crud;

import java.util.ArrayList;
import java.util.Date;

import com.sequoiadb.base.DBCursor;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-11420:并发查询更新后的字段
 * @Author laojingtang
 * @Date 2018.01.04
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2021.07.09
 * @Version 1.10
 */
public class CRUD11420 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "crudcl_11420";
    private DBCollection cl = null;
    private ArrayList<BSONObject> insertRecord = new ArrayList<BSONObject>();
    private int recordNum = 4000;
    private int beginNo = 0;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        cs = sdb.getCollectionSpace(SdbTestBase.csName);
        cl = cs.createCollection(clName);
        insertRecord = CRUDUitls.insertData(cl, beginNo, recordNum);
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor(180 * 1000);
        int beginSetNo = 0;
        int endSetNo = 1000;
        threadSet updateSet = new threadSet(beginSetNo, endSetNo);
        threadQuerySet querySetData = new threadQuerySet(beginSetNo,
                endSetNo);
        int beginSetNo1 = 1000;
        int endSetNo1 = 2000;
        threadSet updateSet1 = new threadSet(beginSetNo1, endSetNo1);
        threadQuerySet querySetData1 = new threadQuerySet(beginSetNo1,
                endSetNo1);
        int beginUnsetNo = 2000;
        int endUnsetNo = 3000;
        threadUnset updateUnset = new threadUnset(beginUnsetNo, endUnsetNo);
        threadQueryUnset queryUnsetData = new threadQueryUnset(beginUnsetNo,
                endUnsetNo);
        int beginUnsetNo1 = 3000;
        int endUnsetNo1 = 4000;
        threadUnset updateUnset1 = new threadUnset(beginUnsetNo1,
                endUnsetNo1);
        threadQueryUnset queryUnsetData1 = new threadQueryUnset(beginUnsetNo1,
                endUnsetNo1);

        es.addWorker(updateSet);
        es.addWorker(querySetData);
        es.addWorker(updateUnset);
        es.addWorker(queryUnsetData);
        es.addWorker(updateSet1);
        es.addWorker(querySetData1);
        es.addWorker(updateUnset1);
        es.addWorker(queryUnsetData1);

        es.run();
        Assert.assertEquals(updateSet.getRetCode(), 0);
        Assert.assertEquals(updateUnset.getRetCode(), 0);
        Assert.assertEquals(querySetData.getRetCode(), 0);
        Assert.assertEquals(queryUnsetData.getRetCode(), 0);
        Assert.assertEquals(updateSet1.getRetCode(), 0);
        Assert.assertEquals(updateUnset1.getRetCode(), 0);
        Assert.assertEquals(querySetData1.getRetCode(), 0);
        Assert.assertEquals(queryUnsetData1.getRetCode(), 0);
        // 校验所有数据
        CRUDUitls.checkRecords(cl, insertRecord, "");
    }

    @AfterClass
    public void tearDown() {
        try {
            if (cs.isCollectionExist(clName)) {
                cs.dropCollection(clName);
            }
        } finally {
            if (sdb != null) {
                sdb.close();
            }
        }
    }

    private class threadSet extends ResultStore {
        private int beginNo;
        private int endNo;

        private threadSet(int beginNo, int endNo) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void set() {

            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "",
                    "");) {
                CollectionSpace cs1 = db
                        .getCollectionSpace(SdbTestBase.csName);
                DBCollection cl = cs1.getCollection(clName);
                for (int i = beginNo; i < endNo; i += 100) {
                    String matcher = "{$and:[{no:{$gte:" + i + "}},{no:{$lt:"
                            + (i + 100) + "}}]}";
                    cl.update(matcher, "{$set:{b:1}}", "", 0);
                }
                for (int i = beginNo; i < endNo; i++) {
                    BSONObject obj = new BasicBSONObject();
                    obj = insertRecord.get(i);
                    obj.put("b", 1);
                }
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                ArrayList<BSONObject> setRecord = new ArrayList<BSONObject>();
                setRecord.addAll(insertRecord.subList(beginNo, endNo));
                CRUDUitls.checkRecords(cl, setRecord, matcher);
            }
        }
    }

    private class threadUnset extends ResultStore {
        private int beginNo;
        private int endNo;

        private threadUnset(int beginNo, int endNo) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void unset() {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "",
                    "");) {
                CollectionSpace cs1 = db
                        .getCollectionSpace(SdbTestBase.csName);
                DBCollection cl = cs1.getCollection(clName);
                for (int i = beginNo; i < endNo; i += 100) {
                    String matcher = "{$and:[{no:{$gte:" + i + "}},{no:{$lt:"
                            + (i + 100) + "}}]}";
                    cl.update(matcher, "{$unset:{a:1}}", "", 0);
                }
                for (int i = beginNo; i < endNo; i++) {
                    BSONObject obj = new BasicBSONObject();
                    obj = insertRecord.get(i);
                    obj.removeField("a");
                }
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                ArrayList<BSONObject> unsetRecord = new ArrayList<BSONObject>();
                unsetRecord.addAll(insertRecord.subList(beginNo, endNo));
                CRUDUitls.checkRecords(cl, unsetRecord, matcher);
            }
        }
    }

    private class threadQuerySet extends ResultStore {
        private int beginNo;
        private int endNo;

        private threadQuerySet(int beginNo, int endNo) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 2)
        private void querySet() {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "",
                    "");) {
                CollectionSpace cs1 = db
                        .getCollectionSpace(SdbTestBase.csName);
                DBCollection cl = cs1.getCollection(clName);
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}},{b:1}]}";
                ArrayList<BSONObject> setQueryRecord = new ArrayList<BSONObject>();
                setQueryRecord.addAll(insertRecord.subList(beginNo, endNo));
                CRUDUitls.checkRecords(cl, setQueryRecord, matcher);
            }
        }
    }

    private class threadQueryUnset extends ResultStore {
        private int beginNo;
        private int endNo;

        private threadQueryUnset(int beginNo, int endNo) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 2)
        private void queryunSet() {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "",
                    "");) {
                CollectionSpace cs1 = db
                        .getCollectionSpace(SdbTestBase.csName);
                DBCollection cl = cs1.getCollection(clName);
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}},{a:{$gte:" + beginNo + "}},{a:{$lt:"
                        + endNo + "}}]}";
                ArrayList<BSONObject> insertRecord1 = new ArrayList<>();
                CRUDUitls.checkRecords(cl, insertRecord1, matcher);
            }
        }
    }

}
