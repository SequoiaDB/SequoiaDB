package com.sequoiadb.testschedule;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.threadexecutor.annotation.ExpectBlock;
import com.sequoiadb.threadexecutor.exception.SchException;

class Jira4174Trans1 {
    private Sequoiadb sdb = null;
    private DBCollection cl = null;

    public Jira4174Trans1(String coordUrl, String csName, String clName) {
        sdb = new Sequoiadb(coordUrl, "", "");
        cl = sdb.getCollectionSpace(csName).getCollection(clName);
    }

    @ExecuteOrder(step = 1, desc = "开启事务1，更新记录R1为R2，强制走索引扫描 ")
    public void updateRecord() {
        sdb.beginTransaction();
        cl.update("{a:1}", "{$set:{a:4}}", Jira4174.HINT);
    }

    @ExecuteOrder(step = 3, desc = "提交事务1 ")
    public void commit() {
        sdb.commit();
    }

    public void tearDown() {
        if (null != sdb && !sdb.isClosed()) {
            sdb.close();
        }
    }
}

class Jira4174Trans2 {
    private Sequoiadb sdb = null;
    private DBCollection cl = null;

    public Jira4174Trans2(String coordUrl, String csName, String clName) {
        sdb = new Sequoiadb(coordUrl, "", "");
        cl = sdb.getCollectionSpace(csName).getCollection(clName);
    }

    public void fun1() {

    }

    @ExecuteOrder(step = 2, desc = "开启事务2，不带条件删除，强制走索引扫描 ")
    @ExpectBlock(confirmTime = 5, contOnStep = 3)
    public void delete() {
        sdb.beginTransaction();
        cl.delete(null, Jira4174.HINT);
    }

    @ExecuteOrder(step = 5, desc = "提交事务1提交之后，事务2中分别走索引扫描及表扫描查询，检查结果")
    public void checkData() {
        fun1();
        DBCursor cusor1 = cl.query();
        while (cusor1.hasNext()) {
            System.out.println(cusor1.getNext());
        }
    }

    public void tearDown() {
        if (null != sdb && !sdb.isClosed()) {
            sdb.close();
        }
    }
}

public class Jira4174 extends SdbTestBase {
    public static final String INDEX_NAME = "Index_Jira4174";
    public static final String INDEX_PATTERN = "{a:1}";
    public static final String HINT = "{'':\"" + Jira4174.INDEX_NAME + "\"}";
    public static final String RECORD1 = "{_id:1, a:1}";
    public static final String RECORD2 = "{_id:1, a:2}";
    public static final String CLNAME = "Jira4174";
    Jira4174Trans1 t1;
    Jira4174Trans2 t2;
    ThreadExecutor te = new ThreadExecutor();
    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() throws SchException {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        // 创建集合并在集合中以a字段创建索引
        DBCollection cl = sdb.getCollectionSpace(csName).createCollection(CLNAME);
        cl.createIndex(INDEX_NAME, INDEX_PATTERN, false, false);

        // 插入记录R1，包含索引字段 
        cl.insert(RECORD1);

        t1 = new Jira4174Trans1(SdbTestBase.coordUrl, csName, CLNAME);
        t2 = new Jira4174Trans2(SdbTestBase.coordUrl, csName, CLNAME);

        te.addWorker(t1);
        te.addWorker(t2);
    }

    @Test
    public void test() throws Exception {
        te.run();
        //te.display();
    }

    @AfterClass
    public void tearDown() {
        tearDownTrans1();
        tearDownTrans2();
        CollectionSpace cs = sdb.getCollectionSpace(csName);
        if (cs.isCollectionExist(CLNAME)) {
            cs.dropCollection(CLNAME);
        }
    }

    private void tearDownTrans1() {
        try {
            if (null != t1) {
                t1.tearDown();
            }
        }
        catch (Exception e) {
        }
    }

    private void tearDownTrans2() {
        try {
            if (null != t2) {
                t2.tearDown();
            }
        }
        catch (Exception e) {
        }
    }
}
