package com.sequoiadb.test.bson;


import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.junit.*;

import java.sql.Timestamp;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.*;
import static org.junit.Assert.assertThat;

/**
 * Created by tanzhaobo on 2017/11/21.
 */
public class BSONTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void BSONTimestampConstructorTest() {
        BSONObject obj = new BasicBSONObject();
        BSONTimestamp ts = new BSONTimestamp(1483200000, 0);
        obj.put("ts", ts);
        String expect = "{ \"ts\" : { \"$ts\" : 1483200000 , \"$inc\" : 0 } }";
        assertEquals(expect, obj.toString());
        System.out.println("ts is: " + obj.toString());

        Date date = new Date(2017 - 1900, 0, 1, 10, 25,59);
        ts = new BSONTimestamp(date);
        obj = new BasicBSONObject();
        obj.put("ts", ts);
        expect = "{ \"ts\" : { \"$ts\" : 1483237559 , \"$inc\" : 0 } }";
        assertEquals(expect, obj.toString());
        System.out.println("ts is: " + obj.toString());

        Timestamp timestamp = new Timestamp(2017 - 1900, 0, 1,
                10, 25,59, 123456000);
        ts = new BSONTimestamp(timestamp);
        obj = new BasicBSONObject();
        obj.put("ts", ts);
        expect = "{ \"ts\" : { \"$ts\" : 1483237559 , \"$inc\" : 123456 } }";
        assertEquals(expect, obj.toString());
        System.out.println("ts is: " + obj.toString());

        cl.insert(obj);
        cursor = cl.query();
        BSONObject record = null;
        try {
            record = cursor.getNext();
        } finally {
            cursor.close();
        }
        assertEquals(obj, record);

        obj = new BasicBSONObject();
        obj.put("ts", timestamp);
        expect = "{ \"ts\" : { \"$ts\" : 1483237559 , \"$inc\" : 123456 } }";
        String str = obj.toString();
        assertEquals(expect, obj.toString());
        System.out.println("ts is: " + obj.toString());

        cl.truncate();
        cl.insert(obj);
        cursor = cl.query();
        record = null;
        try {
            record = cursor.getNext();
        } finally {
            cursor.close();
        }
        ts = (BSONTimestamp)record.get("ts");
        assertEquals(timestamp.getTime() / 1000, ts.getTime());
        assertEquals(timestamp.getNanos(), ts.getInc() * 1000);
    }

    @Test
    public void BSONHashCodeTest() {
        BSONObject obj1 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        BSONObject obj2 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        BSONObject obj3 = new BasicBSONObject().append("b", new BasicBSONObject("b",1)).append("a", 1);
        assertEquals(obj1, obj1);
        assertEquals(obj1, obj2);
        assertEquals(obj2, obj1);
        assertEquals(obj1, obj3);
        assertTrue (obj1.equals(obj1));
        assertTrue (obj1.equals(obj2));
        assertTrue (obj2.equals(obj1));
        assertTrue (obj1.equals(obj3));
        assertTrue (obj3.equals(obj1));
        assertEquals(obj1.hashCode(), obj1.hashCode());
        assertEquals(obj1.hashCode(), obj2.hashCode());
        assertEquals(obj2.hashCode(), obj1.hashCode());
        assertEquals(obj1.hashCode(), obj3.hashCode());
        assertEquals(obj3.hashCode(), obj1.hashCode());
    }

    @Test
    public void BSONInMapTest() {
        BSONObject obj1 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        BSONObject obj2 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        BSONObject obj3 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",3));
        BSONObject obj4 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",4));

        Map<BSONObject, BSONObject> map = new HashMap<BSONObject, BSONObject>();
        map.put(obj1, obj1);
        map.put(obj2, obj2);
        map.put(obj3, obj3);
        map.put(obj4, obj4);
        assertEquals(3, map.size());
    }

}
