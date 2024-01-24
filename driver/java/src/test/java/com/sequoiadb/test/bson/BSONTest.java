package com.sequoiadb.test.bson;


import com.sequoiadb.base.*;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.util.DateInterceptUtil;
import org.junit.*;

import java.sql.Timestamp;
import java.util.*;

import static org.junit.Assert.*;

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
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
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
        // case 1: 2017/01/01
        BSONObject obj = new BasicBSONObject();
        BSONTimestamp ts = new BSONTimestamp(1483200000, 0);
        obj.put("ts", ts);
        String expect = "{ \"ts\" : { \"$ts\" : 1483200000 , \"$inc\" : 0 } }";
        assertEquals(expect, obj.toString());
        System.out.println("ts is: " + obj.toString());

        // case 2: 2017/01/01 10:25:59
        Date date = new Date(2017 - 1900, 0, 1, 10, 25,59);
        ts = new BSONTimestamp(date);
        obj = new BasicBSONObject();
        obj.put("ts", ts);
        expect = "{ \"ts\" : { \"$ts\" : 1483237559 , \"$inc\" : 0 } }";
        assertEquals(expect, obj.toString());
        System.out.println("ts is: " + obj.toString());

        // case 3: 2017/01/01 10:25:59:123456
        Timestamp timestamp = new Timestamp(2017 - 1900, 0, 1,
                10, 25,59, 123456000);
        ts = new BSONTimestamp(timestamp);
        obj = new BasicBSONObject();
        obj.put("ts", ts);
        expect = "{ \"ts\" : { \"$ts\" : 1483237559 , \"$inc\" : 123456 } }";
        assertEquals(expect, obj.toString());
        System.out.println("ts is: " + obj.toString());

        cl.insert(obj);
        BSONObject record = cl.queryOne();
        assertEquals(obj.get("ts"), record.get("ts"));

        // case 4: 2017/01/01 10:25:59:123456
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

    @Test
    public void BSONDecimalINFTest() {
        BSONObject obj = new BasicBSONObject().
                append("MAX", new BSONDecimal("INF")).
                append("max", new BSONDecimal("inf")).
                append("MIN", new BSONDecimal("-INF")).
                append("min", new BSONDecimal("-inf"));
        cl.insert(obj);
    }

    @Test
    public void BSONArrayTest() {
        List list = new ArrayList();
        list.add("1");
        list.add("2");
        BSONObject obj = new BasicBSONObject();
        obj.put("arr", list);
        cl.insert(obj);
    }

    @Test
    public void BSONDateTest() {
        ClientOptions options = new ClientOptions();
        options.setExactlyDate( true );
        Sequoiadb.initClient( options );
        // 带有年月日时分秒的 date 用例
        Date dateCase1 = new Date();
        DateTest("a", dateCase1 );

        // 只带有年月日的 date 用例
        Date dateCase2 = DateInterceptUtil.interceptDate(new Date(),"yyyy-MM-dd");
        DateTest("a", dateCase2);

        // 只带有年月的 date 用例
        Date dateCase3 = DateInterceptUtil.interceptDate(new Date(),"yyyy-MM");
        DateTest("a", dateCase3);

        // 只带有年的 date 用例
        Date dateCase4 = DateInterceptUtil.interceptDate(new Date(),"yyyy");
        DateTest("a", dateCase4);

        // 只带有月的 date 用例
        Date dateCase5 = DateInterceptUtil.interceptDate(new Date(),"MM");
        DateTest("a", dateCase5);

        options.setExactlyDate( false );
        Sequoiadb.initClient( options );
    }
    private void DateTest(String key,Date value){
        BSONObject bsonObject = new BasicBSONObject();
        Date expectDate = DateInterceptUtil.interceptDate(value, "yyyy-MM-dd");
        bsonObject.put(key,value);
        cl.insert(bsonObject);
        DBCursor cursor  = cl.query(bsonObject,null,null,null);
        while (cursor.hasNext()){
            BSONObject b = cursor.getNext();
            Date d = (Date) b.get(key);
            System.out.println(d);
            assertEquals(d, expectDate);
        }
    }

}
