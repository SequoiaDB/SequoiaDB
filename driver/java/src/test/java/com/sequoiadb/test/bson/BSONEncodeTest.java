package com.sequoiadb.test.bson;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.*;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.util.Date;

public class BSONEncodeTest {

    private static final String OID = "_id";

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    private void encodeAndCheck(BSONObject obj, BSONObject extendObj) {
        BSONObject expected = new BasicBSONObject();
        expected.putAll(obj);
        if (extendObj != null) {
            expected.putAll(extendObj);
        }
        if (obj.containsField(OID)) {
            expected.put(OID, obj.get(OID));
        }

        byte[] bytes = BSON.encode(obj, extendObj);
        Assert.assertEquals(expected, BSON.decode(bytes));
    }

    @Test
    public void testCompatibility() {
        BSONObject obj = new BasicBSONObject();
        obj.put("a", 1);

        encodeAndCheck(obj, null);

        obj.put(OID, 1);
        encodeAndCheck(obj, null);

        obj.put(OID, ObjectId.get());
        encodeAndCheck(obj, null);
    }

    @Test
    public void testOid() {
        BSONObject obj = new BasicBSONObject();
        obj.put(OID, 1);
        obj.put("a", 123);

        BSONObject e1 = new BasicBSONObject();
        e1.put("a", null);
        encodeAndCheck(obj, e1);

        BSONObject e2 = new BasicBSONObject();
        e2.put(OID, null);
        encodeAndCheck(obj, e2);

        BSONObject e3 = new BasicBSONObject();
        e3.put(OID, 2);
        encodeAndCheck(obj, e3);

        BSONObject e4 = new BasicBSONObject();
        e4.put(OID, ObjectId.get());
        encodeAndCheck(obj, e4);
    }

    @Test
    public void test() {
        Date date = new Date();
        BSONDate bsonDate = new BSONDate(date.getTime());

        BSONObject obj = new BasicBSONObject();
        obj.put("a", 123);

        BSONObject extendObj = new BasicBSONObject();
        extendObj.put("int", 123);
        extendObj.put("long", 12345L);
        extendObj.put("double", 123.456);
        extendObj.put("string", "hello");
        extendObj.put("null", null);
        extendObj.put("maxKey", new MaxKey());
        extendObj.put("minKey", new MinKey());
        extendObj.put("oid", new ObjectId());
        extendObj.put("true", true);
        extendObj.put("false", false);
        extendObj.put("date", date);
        extendObj.put( "bsonDate", bsonDate );
        extendObj.put("timestamp", new BSONTimestamp((int) (System.currentTimeMillis() / 1000), 1234));
        extendObj.put("decimal", new BSONDecimal("12345678901234567890.09876543210987654321"));
        extendObj.put("object", obj);

        encodeAndCheck(obj, extendObj);
    }

    @Test
    public void testError() {
        BSONObject obj = new BasicBSONObject();
        obj.put("a", 123);
        BSONObject list = new BasicBSONList();
        list.put("0", 123);

        try {
            encodeAndCheck(obj, list);
            Assert.fail("list type is error, shouldn't go here!");
        } catch (IllegalArgumentException e) {
            System.out.println(e.getMessage());
        }

        encodeAndCheck(obj, null);

        encodeAndCheck(list, null);

        try {
            encodeAndCheck(list, list);
            Assert.fail("list type is error, shouldn't go here!");
        } catch (IllegalArgumentException e) {
            System.out.println(e.getMessage());
        }


        try {
            encodeAndCheck(list, obj);
            Assert.fail("obj type is error, shouldn't go here!");
        } catch (IllegalArgumentException e) {
            System.out.println(e.getMessage());
        }
    }
}
