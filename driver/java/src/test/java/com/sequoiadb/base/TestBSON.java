package com.sequoiadb.base;

import org.bson.*;
import org.bson.io.Bits;
import org.bson.types.*;
import org.bson.util.DateInterceptUtil;
import org.bson.util.JSON;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.sql.Timestamp;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.regex.Pattern;

import static org.junit.Assert.*;

public class TestBSON {
    private static BSONObject object;
    private static byte[] objBytes;

    @BeforeClass
    public static void setUpTestCase() {
        Date date = DateInterceptUtil.interceptDate(new Date(), "yyyy-MM-dd");
        BSONDate bsonDate = new BSONDate(date.getTime());

        BSONObject embeddedObj = new BasicBSONObject();
        embeddedObj.put("int", 123);
        embeddedObj.put("long", 12345L);
        embeddedObj.put("double", 123.456);
        embeddedObj.put("string", "hello");
        embeddedObj.put("null", null);
        embeddedObj.put("maxKey", new MaxKey());
        embeddedObj.put("minKey", new MinKey());
        embeddedObj.put("oid", new ObjectId());
        embeddedObj.put("true", true);
        embeddedObj.put("false", false);
        embeddedObj.put("date", date);
        embeddedObj.put( "bsonDate", bsonDate );
        embeddedObj.put("timestamp", new BSONTimestamp((int) (System.currentTimeMillis() / 1000), 1234));
        embeddedObj.put("decimal", new BSONDecimal("12345678901234567890.09876543210987654321"));

        BasicBSONList embeddedArray = new BasicBSONList();
        embeddedArray.put("0", 123);
        embeddedArray.put("1", 12345L);
        embeddedArray.put("2", 123.456);
        embeddedArray.put("3", "hello");
        embeddedArray.put("4", null);
        embeddedArray.put("5", new MaxKey());
        embeddedArray.put("6", new MinKey());
        embeddedArray.put("7", new ObjectId());
        embeddedArray.put("8", true);
        embeddedArray.put("9", false);
        embeddedArray.put("10", date);
        embeddedArray.put("11", new BSONTimestamp((int) (System.currentTimeMillis() / 1000), 1234));
        embeddedArray.put("12", new BSONDecimal("12345678901234567890.09876543210987654321"));
        embeddedArray.put("13", bsonDate);

        Binary binary1 = new Binary(BSON.B_GENERAL, "Hello, world!".getBytes());
        Binary binary2 = new Binary(BSON.B_FUNC, "Hello, world!".getBytes());
        Binary binary3 = new Binary(BSON.B_BINARY, "Hello, world!".getBytes());
        UUID binary4 = UUID.randomUUID();

        BSONObject obj = new BasicBSONObject();
        obj.put("int", 123);
        obj.put("long", 12345L);
        obj.put("double", 123.456);
        obj.put("string", "hello");
        obj.put("null", null);
        obj.put("maxKey", new MaxKey());
        obj.put("minKey", new MinKey());
        obj.put("oid", new ObjectId());
        obj.put("true", true);
        obj.put("false", false);
        obj.put("date", date);
        obj.put("bsonDate", bsonDate);
        obj.put("timestamp", new BSONTimestamp((int) (System.currentTimeMillis() / 1000), 1234));
        obj.put("decimal", new BSONDecimal("12345678901234567890.09876543210987654321"));
        obj.put("binary1", binary1);
        obj.put("binary2", binary2);
        obj.put("binary3", binary3);
        obj.put("binary4", binary4);
        obj.put("object", embeddedObj);
        obj.put("array", embeddedArray);
        obj.put("code", new Code("你好！"));
        obj.put("codewscope", new CodeWScope("hello", new BasicBSONObject()));
        obj.put("regex", Pattern.compile("^a", BSON.regexFlags("i")));
        // TODO Symbol 被解码成 String 导致用例失败，暂时屏蔽
        //obj.put("symbol", new Symbol("你好！"));

        object = obj;
        objBytes = BSON.encode(obj);
    }

    @Test
    public void testNewBSONDecoder() {
        byte[] bytes = objBytes;

        BSONDecoder decoder = new NewBSONDecoder();
        BSONObject decodedObj = decoder.readObject(bytes);

        assertEquals(object, decodedObj);

        Binary binary1 = (Binary) object.get("binary1");
        Binary bin1 = (Binary) decodedObj.get("binary1");
        assertEquals(binary1, bin1);

        Binary binary2 = (Binary) object.get("binary2");
        Binary bin2 = (Binary) decodedObj.get("binary2");
        assertEquals(binary2, bin2);

        Binary binary3 = (Binary) object.get("binary3");
        Binary bin3 = (Binary) decodedObj.get("binary3");
        assertEquals(binary3, bin3);

        UUID binary4 = (UUID) object.get("binary4");
        UUID bin4 = (UUID) decodedObj.get("binary4");
        assertEquals(binary4, bin4);

        BSONDate bsonDate1 = (BSONDate) object.get("bsonDate");
        BSONDate bsonDate2 = (BSONDate) decodedObj.get("bsonDate");
        assertEquals(bsonDate1, bsonDate2);
    }

    @Test
    public void testNewBSONDecoder2() {
        byte[] bytes = objBytes;
        byte[] bytes2 = new byte[bytes.length + 10];
        System.arraycopy(bytes, 0, bytes2, 10, bytes.length);

        BSONDecoder decoder = new NewBSONDecoder();
        BasicBSONCallback callback = new BasicBSONCallback();

        int length = decoder.decode(bytes2, 10, callback);
        assertEquals(Bits.readInt(bytes), length);
    }

    @Test
    public void testNewBSONDecoder3() {
        byte[] bytes = objBytes;
        byte[] bytes2 = new byte[bytes.length + 10];
        System.arraycopy(bytes, 0, bytes2, 10, bytes.length);

        BSONDecoder decoder = new NewBSONDecoder();
        BasicBSONCallback callback = new BasicBSONCallback();

        int length = 0;
        try {
            length = decoder.decode(new ByteArrayInputStream(bytes2, 10, bytes.length), callback);
        } catch (IOException e) {
            e.printStackTrace();
            fail();
        }
        assertEquals(Bits.readInt(bytes), length);
    }

    @Test
    public void testBSONTimestamp() {
        Date srcDate = new Date();
        BSONTimestamp ts = new BSONTimestamp(srcDate);
        Date toDate = ts.toDate();
        assertEquals(srcDate, toDate);

        Timestamp srcTS = new Timestamp(srcDate.getTime());
        srcTS.setNanos(123456000);
        // nanoseconds loss
        BSONTimestamp ts2 = new BSONTimestamp(srcTS);
        Timestamp toTS = ts2.toTimestamp();
        assertEquals(srcTS, toTS);
    }

    @Test
    public void testBSONCodecTimestamp() {
        Timestamp ts = new Timestamp(new Date().getTime());
        ts.setNanos(123456000);

        BSONObject obj = new BasicBSONObject();
        obj.put("ts", ts);

        byte[] bytes = BSON.encode(obj);
        BSONObject obj2 = BSON.decode(bytes);

        BSONTimestamp bts = (BSONTimestamp)obj2.get("ts");
        Timestamp ts2 = bts.toTimestamp();

        assertEquals(ts, ts2);
    }

    @Test
    public void testJSONParseDate() {
        Date date = null;
        try {
            date = new SimpleDateFormat("yyyy-MM-dd").parse("2017-06-14");
        } catch (ParseException e) {
            e.printStackTrace();
            fail();
        }
        java.sql.Date date2 = new java.sql.Date(date.getTime());
        BSONDate date3 = new BSONDate(date.getTime());

        BSONObject obj = new BasicBSONObject();
        obj.put("date", date);
        obj.put("date2", date2);
        obj.put("date3", date3);

        String json = obj.toString();

        BSONObject obj2 = (BSONObject) JSON.parse(json);

        assertEquals(obj, obj2);
    }

    @Test
    public void testBSONDateInBSON(){
        BSONDate bsonDate = new BSONDate(new Date().getTime());

        // case 1: BasicBSONObject
        BSONObject obj = new BasicBSONObject();
        obj.put( "date", bsonDate );

        BSONDate d1 = (BSONDate) obj.get("date");
        assertEquals( bsonDate, d1 );
        Date d2 = (Date) obj.get("date");
        assertEquals( bsonDate, d2 );

        // case 2: BasicBSONList
        BSONObject list = new BasicBSONList();
        list.put( "0", bsonDate );

        BSONDate d3 = (BSONDate) list.get("0");
        assertEquals( bsonDate, d3 );
        Date d4 = (Date) list.get("0");
        assertEquals( bsonDate, d4 );
    }

    @Test
    public void testBSONDate(){

        // case 1: LocalDate
        LocalDate date1 = LocalDate.of( 1899, 12, 31 );
        LocalDate expectDate1 = BSONDate.valueOf( date1 ).toLocalDate();
        Assert.assertEquals( date1.toString(), expectDate1.toString() );

        LocalDate date2 = LocalDate.of( 2022, 1, 1 );
        LocalDate expectDate2 = BSONDate.valueOf( date2 ).toLocalDate();
        Assert.assertEquals( date2.toString(), expectDate2.toString() );

        // case 2: LocalDateTime
        LocalDateTime date3 = LocalDateTime.of( 1, 1, 1, 0, 0, 0, 0 );
        LocalDateTime expectDate3 = BSONDate.valueOf( date3 ).toLocalDateTime();
        Assert.assertEquals( date3.toString(), expectDate3.toString() );

        LocalDateTime date4 = LocalDateTime.of( 2040, 1, 1, 1, 1, 1, 999999999 );
        LocalDateTime actualDate4 = BSONDate.valueOf( date4 ).toLocalDateTime();
        Assert.assertEquals( date4.getYear(), actualDate4.getYear() );
        Assert.assertEquals( date4.getMonth(), actualDate4.getMonth() );
        Assert.assertEquals( date4.getDayOfMonth(), actualDate4.getDayOfMonth() );
        Assert.assertEquals( date4.getHour(), actualDate4.getHour() );
        Assert.assertEquals( date4.getMinute(), actualDate4.getMinute() );
        Assert.assertEquals( date4.getSecond(), actualDate4.getSecond() );
        Assert.assertEquals( 999000000, actualDate4.getNano() );
    }

    @Test
    public void testBSONDateToString() {
        String dateStr1 = "2022-01-01";
        String dateStr2 = "2022-01-01 10:30:01.123";
        String expBSONDateStr1 = "2022-01-01T00:00";
        String expBSONDateStr2 = "2022-01-01T10:30:01.123";

        SimpleDateFormat format1 = new SimpleDateFormat("yyyy-MM-dd");
        DateTimeFormatter format2 = DateTimeFormatter.ofPattern( "yyyy-MM-dd" );
        DateTimeFormatter format3 = DateTimeFormatter.ofPattern( "yyyy-MM-dd HH:mm:ss.SSS" );
        BSONObject obj = new BasicBSONObject();

        try {
            java.util.Date utilDate = format1.parse( dateStr1 );
            java.sql.Date sqlDate = new java.sql.Date( utilDate.getTime() );
            LocalDate localDate = LocalDate.parse( dateStr1, format2 );
            LocalDateTime localDateTime = LocalDateTime.parse( dateStr2, format3 );

            // case 1: normal BSON
            obj.put( "util.Date", utilDate );
            obj.put( "sql.Date", sqlDate );
            obj.put( "LocalDate", BSONDate.valueOf( localDate ) );
            obj.put( "LocalDateTime", BSONDate.valueOf( localDateTime ) );

            String bsonStr = "{ " +
                    "\"util.Date\" : { \"$date\" : \"2022-01-01\" } ," +
                    " \"sql.Date\" : { \"$date\" : \"2022-01-01\" } ," +
                    " \"LocalDate\" : { \"$date\" : \"2022-01-01\" } ," +
                    " \"LocalDateTime\" : { \"$date\" : \"2022-01-01\" }" +
                    " }";
            // Date in BSON
            Assert.assertEquals( bsonStr, obj.toString() );

            // Get date from BSON
            java.util.Date expUtilDate = (java.util.Date) obj.get( "util.Date" );
            java.sql.Date expSqlDate = (java.sql.Date) obj.get( "sql.Date" );
            BSONDate bsonDate1 = (BSONDate) obj.get( "LocalDate" );
            BSONDate bsonDate2 = (BSONDate) obj.get( "LocalDateTime" );
            LocalDate expLd = bsonDate1.toLocalDate();
            LocalDateTime expLdt = bsonDate2.toLocalDateTime();

            Assert.assertEquals( utilDate.toString(), expUtilDate.toString() );
            Assert.assertEquals( dateStr1, expSqlDate.toString() );
            Assert.assertEquals( expBSONDateStr1, bsonDate1.toString() );
            Assert.assertEquals( expBSONDateStr2, bsonDate2.toString() );
            Assert.assertEquals( dateStr1, expLd.toString() );
            Assert.assertEquals( expBSONDateStr2, expLdt.toString() );

            // case 2: BSON encode/decode
            byte[] bytes = BSON.encode( obj );
            BSONObject o = BSON.decode( bytes );

            // Date in BSON
            Assert.assertEquals( bsonStr, o.toString() );

            // Get date from BSON
            expUtilDate = (java.util.Date) o.get( "util.Date" );
            java.util.Date expSqlDate1 = (java.util.Date) o.get( "sql.Date" );
            bsonDate1 = (BSONDate) o.get( "LocalDate" );
            bsonDate2 = (BSONDate) o.get( "LocalDateTime" );
            expLd = bsonDate1.toLocalDate();
            expLdt = bsonDate2.toLocalDateTime();

            Assert.assertEquals( expBSONDateStr1, expUtilDate.toString() );
            Assert.assertEquals( expBSONDateStr1, expSqlDate1.toString() );
            Assert.assertEquals( expBSONDateStr1, bsonDate1.toString() );
            Assert.assertEquals( expBSONDateStr2, bsonDate2.toString() );
            Assert.assertEquals( dateStr1, expLd.toString() );
            Assert.assertEquals( expBSONDateStr2, expLdt.toString() );
        }catch ( ParseException e ) {
            e.printStackTrace();
        }
    }

    @Test
    public void testJSONParseTimestamp() {
        Timestamp ts = new Timestamp(new Date().getTime());
        ts.setNanos(123456000);

        BSONObject obj = new BasicBSONObject();
        obj.put("ts", ts);

        BSONObject obj2 = new BasicBSONObject();
        obj2.put("ts", new BSONTimestamp(ts));

        String json = JSON.serialize(obj);
        BSONObject obj3 = (BSONObject) JSON.parse(json);

        assertEquals(obj2, obj3);
    }

    @Test
    public void testJSONParseBinary() {
        String str = "hello world";
        Binary binary = new Binary(str.getBytes());

        BSONObject obj = new BasicBSONObject();
        obj.put("bin", binary);

        String json = obj.toString();
        BSONObject obj2 = (BSONObject) JSON.parse(json);

        assertEquals(obj, obj2);
    }

    @Test
    public void testNumberLong() {
        BSONObject exp = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775296'}}");
        BSONObject exp2 = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775807'}}");
        assertNotEquals(exp, exp2);

        BSONObject exp3 = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775296'}}");
        BSONObject exp4 = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775296'}}");
        assertEquals(exp3, exp4);
    }

    @Test
    public void testBSONTimestampInc() {
        BSONTimestamp ts1 = new BSONTimestamp(10000, 1000000);
        Assert.assertEquals(10001, ts1.getTime());
        Assert.assertEquals(0, ts1.getInc());

        BSONTimestamp ts2 = new BSONTimestamp(10000, -1);
        Assert.assertEquals(9999, ts2.getTime());
        Assert.assertEquals(999999, ts2.getInc());

        BSONTimestamp ts3 = new BSONTimestamp(10000, 0);
        Assert.assertEquals(10000, ts3.getTime());
        Assert.assertEquals(0, ts3.getInc());

        BSONTimestamp ts4 = new BSONTimestamp(10000, 999999);
        Assert.assertEquals(10000, ts4.getTime());
        Assert.assertEquals(999999, ts4.getInc());

        int time = 1534942305;
        int inc = 123456789;
        int incSec = 123456789 / 1000000;
        int incMSec = 123456789 % 1000000;
        int incMSec2 = 1000000 - incMSec;
        BSONTimestamp ts5 = new BSONTimestamp(time, inc);
        Assert.assertEquals(time + incSec, ts5.getTime());
        Assert.assertEquals(incMSec, ts5.getInc());

        BSONTimestamp ts6 = new BSONTimestamp(time, -inc);
        Assert.assertEquals(time - incSec - 1, ts6.getTime());
        Assert.assertEquals(incMSec2, ts6.getInc());

        BSONTimestamp ts7 = new BSONTimestamp(-time, inc);
        Assert.assertEquals(-time + incSec, ts7.getTime());
        Assert.assertEquals(incMSec, ts7.getInc());

        BSONTimestamp ts8 = new BSONTimestamp(-time, -inc);
        Assert.assertEquals(-time - incSec - 1, ts8.getTime());
        Assert.assertEquals(incMSec2, ts8.getInc());
    }

    @Test
    public void testPutAllUnique(){
        Map<String,String> map = new HashMap<>();
        map.put("1","1");
        map.put("2","2");
        map.put("3","3");

        // test in BasicBSONObject
        // test putAllUnique(BSONObject b)
        BSONObject obj1 = new BasicBSONObject();
        obj1.put("A",1);
        obj1.put("B",2);
        obj1.put("C",3);

        BSONObject obj2 = new BasicBSONObject();
        obj2.put("A",5);
        obj2.putAllUnique(obj1);
        obj1.put("A",5);
        System.out.println(obj2);
        assertEquals(obj1, obj2);

        // test putAllUnique(Map m)
        BSONObject obj3 = new BasicBSONObject();
        obj3.put("1","5");
        obj3.putAllUnique(map);
        map.put("1","5");
        BSONObject obj4 = new BasicBSONObject();
        obj4.putAll(map);
        System.out.println(obj3);
        assertEquals(obj3, obj4);

        // test in BasicBSONObject
        // test putAllUnique(BSONObject b)
        BSONObject objList1 = new BasicBSONList();
        objList1.put("1",1);
        objList1.put("2",2);
        objList1.put("3",3);

        BSONObject objList2 = new BasicBSONList();
        objList2.put("1",5);
        objList2.putAllUnique(objList1);
        objList1.put("1",5);
        assertEquals(objList1, objList2);
        System.out.println(objList2);

        // test putAllUnique(Map m)
        map.put("1","1");
        BSONObject objList3 = new BasicBSONList();
        objList3.put("1","5");
        objList3.putAllUnique(map);
        map.put("1","5");
        BSONObject objList4 = new BasicBSONList();
        objList4.putAll(map);
        assertEquals(objList3, objList4);
        System.out.println(objList3);
    }

    @Test
    public void testJSONParsDate() {
        // case: Date
        String dateStr = "{" +
                " 'date1' : { $date : '1890-01-01T10:30:01.123Z' }, " +
                " 'date2' : { $date : '1890-01-01T10:30:01Z' }, " +
                " 'date3' : { $date : '1890-01-01' }" +
                "}";
        String expDate1 = "1890-01-01T18:35:44.123";
        String expDate2 = "1890-01-01T18:35:44";
        String expDate3 = "1890-01-01T00:00";

        String bsonStr1 = "{ " +
                "\"date1\" : { \"$date\" : \"1890-01-01\" } , " +
                "\"date2\" : { \"$date\" : \"1890-01-01\" } , " +
                "\"date3\" : { \"$date\" : \"1890-01-01\" } " +
                "}";

        BSONObject obj1 = (BSONObject) JSON.parse( dateStr );
        Assert.assertEquals( bsonStr1, obj1.toString() );
        checkDate( obj1, "date1", expDate1 );
        checkDate( obj1, "date2", expDate2 );
        checkDate( obj1, "date3", expDate3 );

        // case: timestamp
        String tsStr = " { 'ts': { $timestamp : '2022-01-01-10.30.01.123456' } }";
        String bsonStr2 = "{ \"ts\" : { \"$ts\" : 1641004201 , \"$inc\" : 123456 } }";
        String expTsStr = "{ $timestamp : 2022-01-01-10.30.01.123456 }";

        BSONObject obj2 = (BSONObject) JSON.parse( tsStr );
        Assert.assertEquals( bsonStr2, obj2.toString() );
        BSONTimestamp ts = (BSONTimestamp) obj2.get( "ts" );
        Assert.assertEquals( expTsStr, ts.toString() );
    }

    private void checkDate( BSONObject obj, String key, String expStr ) {
        BSONDate date = (BSONDate) obj.get( key );
        Assert.assertNotNull( date );
        Assert.assertEquals( expStr, date.toString() );
    }
}
