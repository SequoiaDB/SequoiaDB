package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.SingleCSCLTestCase;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.junit.Assert;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static org.junit.Assert.*;

public class TestInsert extends SingleCSCLTestCase {

    private String OID = "_id";

    @Override
    public void setUp() {
        super.setUp();
        cl.truncate();
    }

    @Override
    public void tearDown() {
        super.tearDown();
        cl.truncate();
    }

    @Test
    public void testInsert() {
        System.out.println("in insert test");
        BSONObject obj = new BasicBSONObject();
        obj.put("str", "hello");
        obj.put("int", 123);
        obj.put("double", 1234.567);

        // case 1:
        cl.insert(obj);
        assertEquals(1, cl.getCount());

        DBCursor cursor = cl.query();
        assertTrue(cursor.hasNext());
        BSONObject res = cursor.getNext();
        assertEquals(res, obj);
        assertFalse(cursor.hasNext());
        cursor.close();

        // case 2:
        BSONObject obj2 = new BasicBSONObject().append("a", 1);
        BSONObject result2 = cl.insert(obj2, DBCollection.FLG_INSERT_RETURN_OID);
        Assert.assertNotNull(result2);
        Assert.assertTrue(result2.get(OID) instanceof ObjectId);
        System.out.println("result2 is: " + result2.toString());

        // case 3:
        BSONObject obj3 = new BasicBSONObject().append("a", 1).append("_id", 1);
        BSONObject result3 = cl.insert(obj3, DBCollection.FLG_INSERT_RETURN_OID);
        Assert.assertNotNull(result3);
        Assert.assertTrue(result3.get(OID) instanceof Integer);
        System.out.println("result3 is: " + result3.toString());
    }

    @Test
    public void testBulkInsert() {
        System.out.println("in bulk insert test");
        final int n = 100;
        List<BSONObject> objs = new ArrayList<BSONObject>(n);
        Random rand = new Random();

        for (int i = 0; i < n; i++) {
            BSONObject obj = new BasicBSONObject();
            obj.put("id", i);
            obj.put("str", "hello");
            obj.put("int", rand.nextInt());
            obj.put("double", 1234.567);

            objs.add(obj);
        }

        // case 1:
        cl.bulkInsert(objs, 0);
        assertEquals(n, cl.getCount());

        BSONObject orderby = new BasicBSONObject();
        orderby.put("id", 1);

        List<BSONObject> res = new ArrayList<BSONObject>(n);
        DBCursor cursor = cl.query(null, null, orderby, null);
        try {
            for (int i = 0; i < n; i++) {
                assertTrue(cursor.hasNext());
                BSONObject obj;
                if (i % 2 == 0) {
                    obj = cursor.getNext();
                } else {
                    byte[] bytes = cursor.getNextRaw();
                    obj = Helper.decodeBSONBytes(bytes);
                }
                res.add(obj);

                BSONObject curObj = cursor.getCurrent();
                assertEquals(obj, curObj);
            }
            assertFalse(cursor.hasNext());
        } finally {
            cursor.close();
        }

        for (int i = 0; i < n; i++) {
            assertEquals(objs.get(i), res.get(i));
        }

        cl.truncate();
        assertEquals(0, cl.getCount());

        // case 2:
        int recordCount = 0;
        List<BSONObject> objectList1 = new ArrayList<BSONObject>();
        objectList1.add(new BasicBSONObject().append("a", 1));
        objectList1.add(new BasicBSONObject().append("a", 2));
        objectList1.add(new BasicBSONObject().append("a", 3));
        recordCount += 3;

        BSONObject result1 = cl.insertRecords(objectList1, DBCollection.FLG_INSERT_RETURN_OID);
        Assert.assertNotNull(result1);
        Assert.assertTrue(result1.get(OID) instanceof BasicBSONList);
        System.out.println("result1 is: " + result1.toString());

        // case 3:
        List<BSONObject> objectList2 = new ArrayList<BSONObject>();
        objectList2.add(new BasicBSONObject().append("a", 1).append("_id", 11));
        objectList2.add(new BasicBSONObject().append("a", 2).append("_id", 11));
        objectList2.add(new BasicBSONObject().append("a", 3).append("_id", 13));
        recordCount += 2;

        BSONObject result2 = cl.insertRecords(objectList2, DBCollection.FLG_INSERT_CONTONDUP | DBCollection.FLG_INSERT_RETURN_OID);
        Assert.assertNotNull(result2);
        Assert.assertTrue(result2.get(OID) instanceof BasicBSONList);
        System.out.println("result2 is: " + result2.toString());

        // case 3:
        try {
            BSONObject result3 = cl.insertRecords(objectList2, DBCollection.FLG_INSERT_RETURN_OID);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_IXM_DUP_KEY.getErrorCode(), e.getErrorCode());
        }

        // case 4:
        BSONObject result4 = cl.insertRecords(objectList2, DBCollection.FLG_INSERT_CONTONDUP);
        Assert.assertEquals(0L, result4.get("InsertedNum"));
        Assert.assertEquals(3L, result4.get("DuplicatedNum"));

        // case 5:
        List<BSONObject> objectList3 = new ArrayList<BSONObject>();
        objectList3.add(new BasicBSONObject().append("a", 1));
        objectList3.add(new BasicBSONObject().append("a", 2));
        objectList3.add(new BasicBSONObject().append("a", 3));
        recordCount += 3;
        List<BSONObject> objectList4 = new ArrayList<BSONObject>();
        objectList4.add(new BasicBSONObject().append("a", 1));
        objectList4.add(new BasicBSONObject().append("a", 2));
        objectList4.add(new BasicBSONObject().append("a", 3));
        recordCount += 3;
        List<BSONObject> objectList5 = new ArrayList<BSONObject>();
        objectList5.add(new BasicBSONObject().append("a", 1));
        objectList5.add(new BasicBSONObject().append("a", 2));
        objectList5.add(new BasicBSONObject().append("a", 3));
        recordCount += 3;

        cl.ensureOID(false);
        BSONObject result5 = cl.insertRecords(objectList3, 0);
        Assert.assertEquals(3L, result5.get("InsertedNum"));

        cl.ensureOID(false);
        BSONObject result6 = cl.insertRecords(objectList4, DBCollection.FLG_INSERT_RETURN_OID);
        Assert.assertNotNull(result6);
        cl.ensureOID(true);
        BSONObject result7 = cl.insertRecords(objectList5, DBCollection.FLG_INSERT_RETURN_OID);
        Assert.assertNotNull(result7);

        assertEquals(recordCount, cl.getCount());

    }

    @Test
    public void testInsertFlags() {
        // case 1:
        BSONObject obj1 = new BasicBSONObject().append("a", 1).append("_id", 1);
        BSONObject obj2 = new BasicBSONObject().append("a", 2).append("_id", 1);
        cl.insert(obj1);
        cl.insert(obj1, DBCollection.FLG_INSERT_CONTONDUP);
        cl.insert(obj2, DBCollection.FLG_INSERT_REPLACEONDUP);
//        cl.insert(obj1, DBCollection.FLG_INSERT_CONTONDUP | DBCollection.FLG_INSERT_REPLACEONDUP);


    }

}
