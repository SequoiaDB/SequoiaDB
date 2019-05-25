package com.sequoiadb.test.db;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;

public class SdbTransaction {
    private static Sequoiadb sdb;
    private static Sequoiadb sdb1;
    private static CollectionSpace cs;
    private static CollectionSpace cs1;
    private static DBCollection cl;
    private static DBCollection cl1;
    private static DBCursor cursor;
    private static String name = "tran_tmp";


    private Boolean isTranOn(Sequoiadb db, DBCollection cl) {
        try {
            db.beginTransaction();
            cl.insert(new BasicBSONObject());
            sdb1.rollback();
            return true;
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DPS_TRANS_DIABLED.getErrorCode()) {
                return false;
            } else {
                throw e;
            }
        }
    }


    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        sdb1 = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb1.isCollectionSpaceExist(name)) {
            sdb1.dropCollectionSpace(name);
            cs1 = sdb1.createCollectionSpace(name);
        } else
            cs1 = sdb1.createCollectionSpace(name);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl1 = cs1.createCollection(name, conf);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb1.dropCollectionSpace(name);
        sdb1.disconnect();
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        System.out.println();
        System.out.println();
    }

    @Test
    public void transaction_begin_rollback_insert() {
        if (false == isTranOn(sdb1, cl1)) {
            System.out.println("transation is disable");
            return;
        }
        System.out.println("Insert record, and then rollback.");
        System.out.println("Transaction begin.");
        sdb.beginTransaction();
        System.out.println("Insert 2 records.");
        BSONObject insertor = new BasicBSONObject();
        insertor.put("name", "tom");
        insertor.put("age", 25);
        insertor.put("addr", "guangzhou");
        BSONObject insertor1 = new BasicBSONObject();
        insertor1.put("name", "sam");
        insertor1.put("age", 27);
        insertor1.put("addr", "shanghai");
        cl.insert(insertor);
        cl.insert(insertor1);
        System.out.println("Before rollback, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        long count;
        BSONObject dummyObj = new BasicBSONObject();
        count = cl.getCount(dummyObj);
        System.out.println("There are " + count + " records.");
        assertEquals(count, 2);
        System.out.println("Rollback.");
        sdb.rollback();
        count = cl.getCount(dummyObj);
        System.out.println("After rollback, there are " + count + " records.");
        assertEquals(0, count);
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        System.out.println("Finish.");
    }

    @Test
    public void transaction_begin_commit_insert() throws InterruptedException {
        if (false == isTranOn(sdb1, cl1)) {
            System.out.println("transation is disable");
            return;
        }
        System.out.println("Insert record, and then commit.");
        System.out.println("Transaction begin.");
        sdb.beginTransaction();
        System.out.println("Insert 2 records.");
        BSONObject insertor = new BasicBSONObject();
        insertor.put("name", "tom");
        insertor.put("age", 25);
        insertor.put("addr", "guangzhou");
        BSONObject insertor1 = new BasicBSONObject();
        insertor1.put("name", "sam");
        insertor1.put("age", 27);
        insertor1.put("addr", "shanghai");
        cl.insert(insertor);
        cl.insert(insertor1);
        System.out.println("Before commit, the records are as below:  ");
        cursor = cl.query();
        long i = 0;
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        long count;
        BSONObject dummyObj = new BasicBSONObject();
        count = cl.getCount(dummyObj);
        System.out.println("Before commit, there are " + count + " records.");
        assertEquals(2, count);
        System.out.println("Commit.");
        sdb.commit();
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are " + count + " records.");
        assertEquals(2, count);
        System.out.println("Finish.");
    }

    /*
    @Test
    public void transaction_begin_rollback_update(){
        if (false == isTranOn(sdb, cl)){
            System.out.println("transation is disable");
            return ;
        }
        System.out.println("Update record, and then rollback.");
        System.out.println("Insert 2 records.");
        BSONObject insertor = new BasicBSONObject();
        insertor.put("name", "tom");
        insertor.put("age", 25);
        insertor.put("addr", "guangzhou");
        BSONObject insertor1 = new BasicBSONObject();
        insertor1.put("name", "sam");
        insertor1.put("age", 27);
        insertor1.put("addr", "shanghai");
        cl.insert(insertor);
        cl.insert(insertor1);
        long count;
        BSONObject dummyObj = new BasicBSONObject();
        count = cl.getCount(dummyObj);
        assertEquals(2,count);
        System.out.println("Before update, the records are as below:  ");
        cursor = cl.query();
        while ( cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        System.out.println("There are "+count+" records.");
        System.out.println("Transaction begin.");
        sdb.beginTransaction();
        System.out.println("Update record.");
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        matcher.put("name", "sam");
        modifier.put("$set",new BasicBSONObject("age",50));
        cl.update(matcher, modifier, null);
        System.out.println("After update record, the records are as below:  ");
        cursor = cl.query();
        while ( cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are "+count+" records.");
        assertEquals(count,2);
        System.out.println("Rollback.");
        sdb.rollback();
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("name", "sam");
        cursor = cl.query(matcher1, null, null, null, 0);
        BSONObject obj = new BasicBSONObject();
        obj = cursor.getNext();
        Boolean ret = obj.get("age").equals(27);
        assertEquals(true, ret);
        count = cl.getCount(dummyObj);
        System.out.println("After rollback, the records are as below:  ");
        cursor = cl.query();
        while ( cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are "+count+" records.");
        assertEquals(count,2);

        System.out.println("Finish.");
    }
*/
    @Test
    public void transaction_begin_commit_update() {
        if (false == isTranOn(sdb1, cl1)) {
            System.out.println("transation is disable");
            return;
        }
        System.out.println("Update record, and then commit.");
        System.out.println("Insert 2 records.");
        BSONObject insertor = new BasicBSONObject();
        insertor.put("name", "tom");
        insertor.put("age", 25);
        insertor.put("addr", "guangzhou");
        BSONObject insertor1 = new BasicBSONObject();
        insertor1.put("name", "sam");
        insertor1.put("age", 27);
        insertor1.put("addr", "shanghai");
        cl.insert(insertor);
        cl.insert(insertor1);
        long count;
        BSONObject dummyObj = new BasicBSONObject();
        count = cl.getCount(dummyObj);
        assertEquals(count, 2);
        System.out.println("Before update, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        System.out.println("There are " + count + " records.");
        System.out.println("Transaction begin.");
        sdb.beginTransaction();
        System.out.println("Update record.");
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        matcher.put("name", "sam");
        modifier.put("$set", new BasicBSONObject("age", 50));
        cl.update(matcher, modifier, null);
        System.out.println("After update record, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are " + count + " records.");
        assertEquals(count, 2);
        System.out.println("Commit.");
        sdb.commit();
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("name", "sam");
        cursor = cl.query(matcher1, null, null, null, 0);
        BSONObject obj = new BasicBSONObject();
        obj = cursor.getNext();
        Boolean ret = obj.get("age").equals(50);
        assertEquals(ret, true);
        count = cl.getCount(dummyObj);
        System.out.println("After commit, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are " + count + " records.");
        assertEquals(count, 2);

        System.out.println("Finish.");
    }

    @Test
    public void transaction_begin_rollback_delete() {
        if (false == isTranOn(sdb1, cl1)) {
            System.out.println("transation is disable");
            return;
        }
        System.out.println("Delete record, and then rollback.");
        System.out.println("Insert 2 records.");
        BSONObject insertor = new BasicBSONObject();
        insertor.put("name", "tom");
        insertor.put("age", 25);
        insertor.put("addr", "guangzhou");
        BSONObject insertor1 = new BasicBSONObject();
        insertor1.put("name", "sam");
        insertor1.put("age", 27);
        insertor1.put("addr", "shanghai");
        cl.insert(insertor);
        cl.insert(insertor1);
        long count;
        BSONObject dummyObj = new BasicBSONObject();
        count = cl.getCount(dummyObj);
        assertEquals(count, 2);
        System.out.println("Before delete, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        System.out.println("There are " + count + " records.");
        System.out.println("Transaction begin.");
        sdb.beginTransaction();
        System.out.println("Delete all the records.");
        cl.delete(dummyObj);
        count = cl.getCount(dummyObj);
        System.out.println("After delete records, there are " + count + " records.");
        assertEquals(count, 0);
        System.out.println("Rollback.");
        sdb.rollback();
        count = cl.getCount(dummyObj);
        System.out.println("After rollback, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are " + count + " records.");
        assertEquals(2, count);

        System.out.println("Finish.");
    }

    @Test
    public void transaction_begin_commit_delete() {
        if (false == isTranOn(sdb1, cl1)) {
            System.out.println("transation is disable");
            return;
        }
        System.out.println("Delete record, and then commit.");
        System.out.println("Insert 2 records.");
        BSONObject insertor = new BasicBSONObject();
        insertor.put("name", "tom");
        insertor.put("age", 25);
        insertor.put("addr", "guangzhou");
        BSONObject insertor1 = new BasicBSONObject();
        insertor1.put("name", "sam");
        insertor1.put("age", 27);
        insertor1.put("addr", "shanghai");
        cl.insert(insertor);
        cl.insert(insertor1);
        long count;
        BSONObject dummyObj = new BasicBSONObject();
        count = cl.getCount(dummyObj);
        assertEquals(count, 2);
        System.out.println("Before delete, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        System.out.println("There are " + count + " records.");
        System.out.println("Transaction begin.");
        sdb.beginTransaction();
        System.out.println("Delete all the records.");
        cl.delete(dummyObj);
        count = cl.getCount(dummyObj);
        System.out.println("After delete records, there are " + count + " records.");
        assertEquals(count, 0);
        System.out.println("Commit.");
        sdb.commit();
        count = cl.getCount(dummyObj);
        System.out.println("After commit, there are " + count + " records.");
        assertEquals(count, 0);

        System.out.println("Finish.");
    }

    @Test
    public void transaction_begin_rollback_update() {
        if (false == isTranOn(sdb1, cl1)) {
            System.out.println("transation is disable");
            return;
        }
        System.out.println("Update record, and then rollback.");
        System.out.println("Insert 2 records.");
        BSONObject insertor = new BasicBSONObject();
        insertor.put("name", "tom");
        insertor.put("age", 25);
        insertor.put("addr", "guangzhou");
        BSONObject insertor1 = new BasicBSONObject();
        insertor1.put("name", "sam");
        insertor1.put("age", 27);
        insertor1.put("addr", "shanghai");
        cl.insert(insertor);
        cl.insert(insertor1);
        long count;
        BSONObject dummyObj = new BasicBSONObject();
        count = cl.getCount(dummyObj);
        assertEquals(2, count);
        System.out.println("Before update, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        System.out.println("There are " + count + " records.");
        System.out.println("Transaction begin.");
        sdb.beginTransaction();
        System.out.println("Update record.");
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        matcher.put("name", "sam");
        modifier.put("$set", new BasicBSONObject("age", 50));
        cl.update(matcher, modifier, null);
        System.out.println("After update record, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are " + count + " records.");
        assertEquals(count, 2);
        System.out.println("Rollback.");
        sdb.rollback();
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("name", "sam");
        cursor = cl.query(matcher1, null, null, null, 0);
        BSONObject obj = new BasicBSONObject();
        obj = cursor.getNext();
        System.out.println(obj.toString());
        Boolean ret = obj.get("age").equals(27);
        assertEquals(true, ret);
        count = cl.getCount(dummyObj);
        System.out.println("After rollback, the records are as below:  ");
        cursor = cl.query();
        while (cursor.hasNext())
            System.out.println(cursor.getNext().toString());
        count = cl.getCount(dummyObj);
        System.out.println("There are " + count + " records.");
        assertEquals(count, 2);

        System.out.println("Finish.");
    }

}
