package com.sequoiadb.test.bug;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.*;


public class Bug_CL_InsertOld {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {

    }

    @After
    public void tearDown() throws Exception {
        cl.delete("");
    }

    @Test
    public void testInsertOid() {
        System.out.println("begin to test testInsertOid ...");
        String str = "{ \"_id\" : { \"$oid\" : \"000102030405060708090a0b\" } }";
        byte[] arr = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        BSONObject obj = new BasicBSONObject();
        ObjectId id = new ObjectId(arr);
        obj.put("_id", id);

        cl.insert(obj);
        BSONObject obj1 = new BasicBSONObject();
        obj1.put("hello", "world");

        cursor = cl.query(obj, obj1, null, null);
        DBCursor cursor2 = cl.query();
        BSONObject rec = null;
        while (cursor2.hasNext()) {
            rec = cursor2.getNext();
            break;
        }
        System.out.println("str is   : " + str);
        System.out.println("record is: " + rec.toString());
        assertTrue(rec.toString().equals(str));
        System.out.println("finish testInsertOid\n");

        // { "_id" : { "$oid" : "000102030405060708090a0b"}}
        // { "_id" : { "$oid" : "03020100070605040b0a0908"}}
    }

    @Test
    public void testBulkInsertOid() {
        System.out.println("begin to test testBulkInsertOid ...");
        String str1 = "{ \"_id\" : { \"$oid\" : \"000102030405060708090a0b\" } }";
        String str2 = "{ \"_id\" : { \"$oid\" : \"0c0d0e0f1011121314151617\" } }";
        List<BSONObject> list = new ArrayList<BSONObject>();
        byte[] arr1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        byte[] arr2 = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
        BSONObject obj1 = new BasicBSONObject();
        BSONObject obj2 = new BasicBSONObject();
        ObjectId id1 = new ObjectId(arr1);
        ObjectId id2 = new ObjectId(arr2);

        obj1.put("_id", id1);
        obj2.put("_id", id2);
        list.add(obj1);
        list.add(obj2);

        // test
        cl.bulkInsert(list, 0);
        // check
        DBCursor cursor1 = cl.query(obj1, null, null, null);
        BSONObject rec1 = null;
        int i = 0;
        while (cursor1.hasNext()) {
            rec1 = cursor1.getNext();
            i++;
            if (i > 1)
                break;
        }
        if (null == rec1) fail();
        assertEquals(1, i);
        assertTrue(rec1.toString().equals(str1));

        DBCursor cursor2 = cl.query(obj2, null, null, null);
        BSONObject rec2 = null;
        i = 0;
        while (cursor2.hasNext()) {
            rec2 = cursor2.getNext();
            i++;
            if (i > 1)
                break;
        }
        assertEquals(1, i);
        if (null == rec2) fail();
        assertTrue(rec2.toString().equals(str2));
        System.out.println("str1 is: " + str1);
        System.out.println("rec1 is: " + rec1.toString());
        System.out.println("str2 is: " + str2);
        System.out.println("rec2 is: " + rec2.toString());
        System.out.println("finish testBulkInsertOid\n");

//      rec1 is: { "_id" : { "$oid" : "03020100070605040b0a0908"}}
//		rec1 is: { "_id" : { "$oid" : "0f0e0d0c1312111017161514"}}
    }

    @Test
    public void testAggregateOld() {
        System.out.println("begin to test testAggregateOld ...");
        String str = "{ \"_id\" : { \"$oid\" : \"000102030405060708090a0b\" } }";
        byte[] arr1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        byte[] arr2 = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
        BSONObject obj1 = new BasicBSONObject();
        BSONObject obj2 = new BasicBSONObject();
        ObjectId id1 = new ObjectId(arr1);
        ObjectId id2 = new ObjectId(arr2);
        // prepare 2 record
        obj1.put("_id", id1);
        obj2.put("_id", id2);
        cl.insert(obj1);
        cl.insert(obj2);

        List<BSONObject> list = new ArrayList<BSONObject>();
        BSONObject project = new BasicBSONObject();
        project.put("$project", new BasicBSONObject("_id", 1));
        BSONObject match = new BasicBSONObject();
        match.put("$match", new BasicBSONObject("_id", new BasicBSONObject("$et", id1)));

        // test
        list.add(project);
        list.add(match);
        cursor = cl.aggregate(list);
        // check
        BSONObject rec = null;
        int i = 0;
        while (cursor.hasNext()) {
            rec = cursor.getNext();
            i++;
            if (i > 1)
                break;
        }
        if (null == rec)
            fail();
        assertEquals(1, i);
        assertTrue(rec.toString().equals(str));
        System.out.println("str is   : " + str);
        System.out.println("result is: " + rec.toString());
        System.out.println("finish testAggregateOld\n");
    }

}
