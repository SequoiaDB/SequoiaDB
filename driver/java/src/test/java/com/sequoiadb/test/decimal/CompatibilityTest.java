package com.sequoiadb.test.decimal;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.DecimalTmpA;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.*;
import org.bson.util.JSON;
import org.junit.*;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Pattern;

import static org.junit.Assert.assertTrue;

/**
 * @author tanzhaobo
 * @brief 测试加入BSONDecimal之后：
 * 1、Java驱动涉及解析BSON的接口是否会受到加入BSONDecimal的影响；
 * 2、由Java驱动插入的BSON能否在其它驱动成功解析；
 * 3、在其它驱动插入的BSON能否在Java驱动中成功解析。
 */
public class CompatibilityTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cur;

    @BeforeClass
    public static void beforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            cs = sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        // cl
        if (cs.isCollectionExist(Constants.TEST_CL_NAME_1)) {
            cl = cs.getCollection(Constants.TEST_CL_NAME_1);
        } else {
            cl = cs.createCollection(Constants.TEST_CL_NAME_1,
                new BasicBSONObject().append("ReplSize", 0));
        }

    }

    @AfterClass
    public static void afterClass() throws Exception {
        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            e.printStackTrace();
        }
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
        cl.truncate();
    }

    @Test
//	@Ignore
    public void tmpTest() {
        // case 1: specify integer value
        BSONDecimal decimal1 = new BSONDecimal("1234.56789", 10, 5);
        System.out.println("decimal1 is: " + decimal1);

        BSONObject obj = new BasicBSONObject("a", decimal1);

        // obj = new BasicBSONObject("a", new BSONTimestamp(1000, 123456));

        System.out.println("inserted record is: " + obj);
        cl.insert(obj);
        cur = cl.query();
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("queried record is: " + obj);

        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("a");
        System.out.println("retDecimal1 is: " + retDecimal1);

        Assert.assertEquals(decimal1.getPrecision(), retDecimal1.getPrecision());
        Assert.assertEquals(decimal1.getScale(), retDecimal1.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal1.getValue()).compareTo(new BigDecimal(retDecimal1.getValue())));

        System.out.println("finish");
    }

    @Test
//	@Ignore
    public void tmpTest2() {
        List<String> list = new ArrayList<String>();
        list.add("a");
        list.add("b");
        list.add("c");
        BSONObject obj = new BasicBSONObject();
        obj.put("list", list);

        // obj = new BasicBSONObject("a", new BSONTimestamp(1000, 123456));

        System.out.println("inserted record is: " + obj);
        cl.insert(obj);
        cur = cl.query();
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("queried record is: " + obj);
    }

    /**
     * Java插入，sdb shell读取
     */
    @Test
//	@Ignore
    public void javaInsertCQueryTest() {
        BSONDecimal decimal1 = new BSONDecimal("-1234.56789", 10, 5);
        System.out.println("decimal1 is: " + decimal1);

        BSONObject obj = new BasicBSONObject("a", decimal1);
        System.out.println("inserted record is: " + obj);
        cl.insert(obj);
        cur = cl.query();
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("queried record is: " + obj);
    }

    @Test
    @Ignore
    public void CInsertJavaQueryTest() {
        cur = cl.query();
        assertTrue(cur.hasNext());
        BSONObject obj = cur.getNext();
        // { "a" : [ { "b" : { "$decimal" : "1234567.00009888"}} , { "c" : { "$decimal" : "123.45679" , "$precision" : [ 10 , 5]}}]}
        System.out.println("queried record is: " + obj);
    }

    //// testing api

    /**
     * this api is in com.sequoiadb.util
     */
    @Test
    @Ignore
    public void api_bsonEndianConvert() {
        // no need to do here, when we run the test case in power, we know the result
    }

    @Test
    @Ignore
    public void api_BasicBSONObject_toString() {
        // we had test it in other test case
    }

    @Test
//	@Ignore
    public void api_BasicBSONObject_equals() {
        BSONObject obj1 = new BasicBSONObject().append("a", 1);
        BSONObject obj2 = new BasicBSONObject().append("a", 2);
        if (obj1.equals(obj2)) {
            System.out.println("yes");
        } else {
            System.out.println("false");
        }

        BSONDecimal d1 = new BSONDecimal("123.456", 10, 5);
        BSONDecimal d2 = new BSONDecimal("123.456", 10, 5);
        if (d1.equals(d2)) {
            System.out.println("yes");
        } else {
            System.out.println("false");
        }

    }

    @Test
    @Ignore
    public void api_BasicBSONObject_BasicTypeWrite() {
        // no need to care about
    }

    @Test
//	@Ignore
    public void api_BasicBSONObject_as() {
        ArrayList<Integer> list = new ArrayList<Integer>();
        list.add(0);
        list.add(1);
        list.add(2);
        int num = 10;
        String str = "abc";
        ObjectId oid = new ObjectId();
        BSONTimestamp ts = new BSONTimestamp(1000, 1000);
        BSONDecimal decimal = new BSONDecimal("123.456");
        BigDecimal bd = new BigDecimal("12345.56789");

        BSONObject obj1 = new BasicBSONObject("case1", "test_in_java").
            append("fieldZ", bd).
            append("fieldA", num).
            append("fieldB", str).
            append("fieldC", oid).
            append("fieldD", ts).
            append("fieldE", decimal).
            append("fieldF", list);

        // case 1
        DecimalTmpA myObj = new DecimalTmpA();
        cl.save(myObj);
        cur = cl.query();
        assertTrue(cur.hasNext());
        BSONObject retDoc = cur.getNext();
        System.out.println("case3: query return record is: " + retDoc);
        DecimalTmpA retObj = null;
        try {
            retObj = retDoc.as(DecimalTmpA.class);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail();
        }
        // check
        Assert.assertEquals(myObj.getFieldA(), retObj.getFieldA());
        Assert.assertEquals(myObj.getFieldB(), retObj.getFieldB());
        Assert.assertEquals(myObj.getFieldC(), retObj.getFieldC());
        Assert.assertEquals(myObj.getFieldD(), retObj.getFieldD());
        Assert.assertEquals(myObj.getFieldE(), retObj.getFieldE());
        Assert.assertEquals(myObj.getFieldF(), retObj.getFieldF());
        Assert.assertEquals(myObj.getFieldZ(), retObj.getFieldZ());

        // case 2: as before insert
        retObj = null;
        try {
            retObj = obj1.as(DecimalTmpA.class);
            System.out.println("case 2 result: " + retObj);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail();
        }
        // check
        Assert.assertEquals(num, retObj.getFieldA());
        Assert.assertEquals(str, retObj.getFieldB());
        Assert.assertEquals(oid.toString(), retObj.getFieldC().toString());
        Assert.assertEquals(ts, retObj.getFieldD());
        Assert.assertEquals(decimal, retObj.getFieldE());
        Assert.assertEquals(list, retObj.getFieldF());
        Assert.assertEquals(bd, retObj.getFieldZ());

        // case 3: as after query
        obj1.removeField("case2");
        obj1.put("case3", "test_in_java");
        cl.insert(obj1);
        cur = cl.query(new BasicBSONObject("case3", "test_in_java"),
            null, null, null);
        assertTrue(cur.hasNext());
        retDoc = cur.getNext();
        System.out.println("case 3: query return record is: " + retDoc);
        try {
            retObj = retDoc.as(DecimalTmpA.class);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail();
        }
        // check
        Assert.assertEquals(num, retObj.getFieldA());
        Assert.assertEquals(str, retObj.getFieldB());
        Assert.assertEquals(oid.toString(), retObj.getFieldC().toString());
        Assert.assertEquals(ts, retObj.getFieldD());
        Assert.assertEquals(decimal, retObj.getFieldE());
        Assert.assertEquals(list, retObj.getFieldF());
        Assert.assertEquals(bd, retObj.getFieldZ());

    }

    @Test
//	@Ignore
    public void api_BasicBSONObject_asMap() {
        BasicBSONObject obj = new BasicBSONObject("a", new BSONTimestamp(1000, 1000)).append("b", new BSONDecimal("1.234"));
        HashMap map = (HashMap) obj.asMap();
        BSONTimestamp ts = (BSONTimestamp) map.get("a");
        BSONDecimal decimal = (BSONDecimal) map.get("b");
        System.out.println("ts: " + ts);
        System.out.println("decimal: " + decimal);
    }

    @Test
//	@Ignore
    public void api_BasicBSONObject_typeToBson() {
        DecimalTmpA a = new DecimalTmpA();
        DecimalTmpA b = new DecimalTmpA();

        a.setFieldA(99);
        a.setFieldB("hello");
        a.setFieldC(new ObjectId());
        a.setFieldD(new BSONTimestamp(1000, 1000));
        a.setFieldE(new BSONDecimal("123.45678"));
        System.out.println("the input TmpA is: " + a);
        try {
            cl.save(a);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("faied to save obj: " + a);
        }
        cur = cl.query(new BasicBSONObject("fieldA", 99), null, null, null);
        Assert.assertTrue(cur.hasNext());
        BSONObject obj = cur.getNext();
        System.out.println("the queried recored is: " + obj);
        Assert.assertEquals(99, obj.get("fieldA"));
        BigDecimal original = new BigDecimal(a.getFieldE().getValue());
        BSONDecimal decimal = (BSONDecimal) obj.get("fieldE");
        BigDecimal after = new BigDecimal(decimal.getValue());
        Assert.assertEquals(0, original.compareTo(after));

        // case 2: 
        b.setFieldA(1000);
        b.setFieldB("hello");
        b.setFieldC(new ObjectId());
        b.setFieldD(new BSONTimestamp(1000, 1000));
        b.setFieldE(new BSONDecimal("1.2332456", 10, 9));

        ArrayList<DecimalTmpA> list = new ArrayList<DecimalTmpA>();
        list.add(a);
        list.add(b);
        try {
            cl.save(list);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("faied to save list: " + list);
        }
        cur = cl.query(new BasicBSONObject("fieldA", 1000), null, null, null);
        Assert.assertTrue(cur.hasNext());
        obj = cur.getNext();
        decimal = (BSONDecimal) obj.get("fieldE");
        original = new BigDecimal(b.getFieldE().getValue());
        after = new BigDecimal(decimal.getValue());
        Assert.assertEquals(0, original.compareTo(after));
        Assert.assertEquals(10, decimal.getPrecision());
        Assert.assertEquals(9, decimal.getScale());
    }

    @Test
    @Ignore
    public void api_BasicBSONList_toMap() {
        // no need to care about
    }

    @Test
    @Ignore
    public void api_BasicBSONList_as() {
        // no need to care about
    }

    @Test
    @Ignore
    public void api_BasicBSONList_asList() {
        // no need to care about
    }

    @Test
//	@Ignore
    public void api_JSONParse() {
        String str1 = "{a:{\"$decimal\" : \"789.1234\"}}";
        String str2 = "{b:{\"$decimal\" : \"123.4567\", \"$precision\" :[10, 5]}}";
        String str3 = "{c:{\"$regex\":\"^1111\", \"$options\":\"i\"}}";

        BSONObject obj1 = (BSONObject) JSON.parse(str1);
        BSONObject obj2 = (BSONObject) JSON.parse(str2);
        BSONObject obj3 = (BSONObject) JSON.parse(str3);

        cl.insert(obj1);
        cl.insert(obj2);
        cl.insert(obj3);

        System.out.println("obj1 is: " + obj1);
        System.out.println("obj2 is: " + obj2);
        System.out.println("obj2 is: " + obj3);
    }

    @Test
//	@Ignore
    public void api_JSONParse_all_type() {
        BSONObject obj = null;
        try {
            cl.truncate();
        } catch (BaseException e) {
            Assert.fail("failed to clean up before running test case");
        }
        String str_num = "{k_int:1, k_long:9223372036854775807, k_float:3.14}";
        String str_decimal = "{k_decimal:{$decimal:\"12345.06789\", $precision:[10, 5]}, k_decimal2:{$decimal:\"111.111\"}}";
        String str_str = "{k_string:\"hello world\"}";
        String str_oid = "{ k_oid : { \"$oid\" : \"123abcd00ef12358902300ef\" } }";
        String str_bool = "{k_bool:true}";
        String str_date = "{k_date:{ \"$date\" : \"2012-01-01\" }}";
        String str_ts = "{ k_timestamp : { \"$timestamp\" : \"2012-01-01-13.14.26.124233\" } }";
        String str_bin = "{ k_bin : { \"$binary\" : \"aGVsbG8gd29ybGQ=\", \"$type\" : \"1\" } }";
        String str_regex = "{ k_regex : { \"$regex\" : \"^张\", \"$options\" : \"i\" } }";
        String str_obj = "{k_object:{a:1,b:{b0:0,b1:1},c:[0,1,2]},k_array:[0,1,2,3,4,5]}";
        String str_arr = "{k_arr:[0,1,2], k_arr2:[{a:1,b:[0, 1, 2]}]}";
        String str_null = "{k_null:null}";

        BSONObject obj1 = (BSONObject) JSON.parse(str_num);
        BSONObject obj2 = (BSONObject) JSON.parse(str_decimal);
        BSONObject obj3 = (BSONObject) JSON.parse(str_str);
        BSONObject obj4 = (BSONObject) JSON.parse(str_oid);
        BSONObject obj5 = (BSONObject) JSON.parse(str_bool);
        BSONObject obj6 = (BSONObject) JSON.parse(str_date);
        BSONObject obj7 = (BSONObject) JSON.parse(str_ts);
        BSONObject obj8 = (BSONObject) JSON.parse(str_bin);
        BSONObject obj9 = (BSONObject) JSON.parse(str_regex);
        BSONObject obj10 = (BSONObject) JSON.parse(str_obj);
        BSONObject obj11 = (BSONObject) JSON.parse(str_arr);
        BSONObject obj12 = (BSONObject) JSON.parse(str_null);

        System.out.println("obj1 is: " + obj1);
        System.out.println("obj2 is: " + obj2);
        System.out.println("obj3 is: " + obj3);
        System.out.println("obj4 is: " + obj4);
        System.out.println("obj5 is: " + obj5);
        System.out.println("obj6 is: " + obj6);
        System.out.println("obj7 is: " + obj7);
        System.out.println("obj8 is: " + obj8);
        System.out.println("obj9 is: " + obj9);
        System.out.println("obj10 is: " + obj10);
        System.out.println("obj11 is: " + obj11);
        System.out.println("obj12 is: " + obj12);

        cl.insert(obj1);
        cl.insert(obj2);
        cl.insert(obj3);
        cl.insert(obj4);
        cl.insert(obj5);
        cl.insert(obj6);
        cl.insert(obj7);
        cl.insert(obj8);
        cl.insert(obj9);
        cl.insert(obj10);
        cl.insert(obj11);
        cl.insert(obj12);

        // number
        cur = cl.query(obj1, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        Assert.assertEquals(1, obj.get("k_int"));
        Assert.assertEquals(9223372036854775807L, obj.get("k_long"));
        Assert.assertEquals(3.14, obj.get("k_float"));

        // decimal
        cur = cl.query(obj2, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        BSONDecimal decimal1 = null;
        BSONDecimal decimal2 = null;
        try {
            // what i care about is the type
            decimal1 = (BSONDecimal) obj.get("k_decimal");
            decimal2 = (BSONDecimal) obj.get("k_decimal2");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to BSONDecimal");
        }

        // string
        cur = cl.query(obj3, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        Assert.assertEquals("hello world", obj.get("k_string"));

        // oid
        cur = cl.query(obj4, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        ObjectId id = null;
        try {
            id = (ObjectId) obj.get("k_oid");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to ObjectId");
        }

        // bool
        cur = cl.query(obj5, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        Assert.assertEquals(true, obj.get("k_bool"));

        // date
        cur = cl.query(obj6, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        Date date = null;
        try {
            date = (Date) obj.get("k_date");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to Date");
        }

        // timestamp
        cur = cl.query(obj7, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        BSONTimestamp ts = null;
        try {
            ts = (BSONTimestamp) obj.get("k_timestamp");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to BSONTimestamp");
        }

        // binary
        cur = cl.query(obj8, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        Binary bin = null;
        try {
            bin = (Binary) obj.get("k_bin");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to Binary");
        }

        // regex
        String str_regex_query = "{ k_regex : { \"$et\": { \"$regex\" : \"^张\", \"$options\" : \"i\" } } }";
        BSONObject obj9query = (BSONObject) JSON.parse(str_regex_query);
        cur = cl.query(obj9query, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        Pattern pattern = null;
        try {
            pattern = (Pattern) obj.get("k_regex");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to Pattern");
        }

        // object
        cur = cl.query(obj10, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        BSONObject subObj = null;
        BSONObject subObj2 = null;
        BSONObject subObj3 = null;
        BSONObject subArr = null;
        try {
            subObj = (BasicBSONObject) obj.get("k_object");
            subObj2 = (BasicBSONObject) subObj.get("b");
            subObj3 = (BasicBSONList) subObj.get("c");
            subArr = (BasicBSONList) obj.get("k_array");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to BasicBSONObject");
        }
        Assert.assertEquals(1, subObj.get("a"));
        Assert.assertEquals(0, subObj2.get("b0"));
        Assert.assertEquals(1, subObj2.get("b1"));
        Assert.assertEquals(0, subArr.get("0"));

        // array
        cur = cl.query(obj11, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        BSONObject subArr1 = null;
        BSONObject subArr2 = null;
        try {
            subArr1 = (BasicBSONList) obj.get("k_arr");
            subArr2 = (BasicBSONList) obj.get("k_arr2");
            subObj = (BasicBSONObject) subArr2.get("0");
            subArr = (BasicBSONList) subObj.get("b");
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail("failed to transform to BasicBSONList");
        }
        Assert.assertEquals(0, subArr1.get("0"));
        Assert.assertEquals(1, subArr1.get("1"));
        Assert.assertEquals(2, subArr1.get("2"));

        Assert.assertEquals(1, subObj.get("a"));

        Assert.assertEquals(0, subArr.get("0"));
        Assert.assertEquals(1, subArr.get("1"));
        Assert.assertEquals(2, subArr.get("2"));

        // null
        cur = cl.query(obj12, null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        Assert.assertEquals(null, obj.get("k_null"));
    }


}
