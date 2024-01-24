package com.sequoiadb.test.decimal;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.DecimalCommon;
import com.sequoiadb.test.common.DecimalPair;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.junit.*;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Random;

import static org.junit.Assert.assertTrue;

/**
 * @author tanzhaobo
 * @brief 测试数据正确性
 */
public class CorrectnessTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cur;
    private static Random rand;

    @BeforeClass
    public static void beforeClass() throws Exception {
        rand = new Random();
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        // cl
        cl = cs.createCollection(Constants.TEST_CL_NAME_1,
            new BasicBSONObject().append("ReplSize", 0));
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
    @Ignore
    public void tmpTest() {
        String str = "2";
        BSONDecimal d = new BSONDecimal(str, 5, 2);
        System.out.println("d is: " + d.getValue());
    }

    @Test
    @Ignore
    public void Test() {
        // case 1: specify integer value
        String str = "-1234.56789";
        str = null;
        BSONDecimal decimal1 = new BSONDecimal(str, 10, 5);
        System.out.println("decimal1 is: " + decimal1);

        BSONObject obj = new BasicBSONObject("a", decimal1);
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

    /**
     * 测试插入整数
     */
    @Test
//	@Ignore
    public void numberTest() {
        // case 1: specify integer value
        BSONDecimal decimal1 = DecimalCommon.genIntegerBSONDecimal(true, true);
        BSONDecimal decimal2 = DecimalCommon.genIntegerBSONDecimal(true, false);
        BSONDecimal decimal3 = DecimalCommon.genIntegerBSONDecimal(false, false);
        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);

        BSONObject obj = new BasicBSONObject("a", decimal1).append("b", decimal2).append("c", decimal3).append("case1", "test_in_java");
        System.out.println("inserted record is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("queried record is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("a");
        BSONDecimal retDecimal2 = (BSONDecimal) obj.get("b");
        BSONDecimal retDecimal3 = (BSONDecimal) obj.get("c");
        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);

        Assert.assertEquals(decimal1.getPrecision(), retDecimal1.getPrecision());
        Assert.assertEquals(decimal1.getScale(), retDecimal1.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal1.getValue()).compareTo(new BigDecimal(retDecimal1.getValue())));

        Assert.assertEquals(decimal2.getPrecision(), retDecimal2.getPrecision());
        Assert.assertEquals(decimal2.getScale(), retDecimal2.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal2.getValue()).compareTo(new BigDecimal(retDecimal2.getValue())));

        Assert.assertEquals(decimal3.getPrecision(), retDecimal3.getPrecision());
        Assert.assertEquals(decimal3.getScale(), retDecimal3.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal3.getValue()).compareTo(new BigDecimal(retDecimal3.getValue())));
        System.out.println("finish");
    }

    /**
     * 测试插入小数
     */
    @Test
    @Ignore
    public void decimalTest() {
        // we have do it in BSONTest.java::nestingBSONDecimalInArrayTest
    }

    /**
     * 测试插入不带整数部分的小数，如："1.123","1.0123e5"
     */
    @Test
//	@Ignore
    public void decimalWithIntegerPartTest() {
        // case 1:
        BSONDecimal decimal1 = DecimalCommon.genBSONDecimal(false, true, true, rand.nextInt(1000));
        BSONDecimal decimal2 = DecimalCommon.genBSONDecimal(false, true, true, -rand.nextInt(1000));
        BSONDecimal decimal3 = DecimalCommon.genBSONDecimal(false, true, false, 0);
        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);

        BSONObject obj =
            new BasicBSONObject("case1", "test_in_java").append("f1", decimal1).
                append("f2", decimal2).append("f3", decimal3);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("f1");
        BSONDecimal retDecimal2 = (BSONDecimal) obj.get("f2");
        BSONDecimal retDecimal3 = (BSONDecimal) obj.get("f3");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);

        System.out.println("1");
        System.out.println("decimal: " + decimal1);
        System.out.println("retDecimal: " + retDecimal1);
        System.out.println("bigDecimal: " + new BigDecimal(decimal1.getValue()));
        Assert.assertEquals(decimal1.getPrecision(), retDecimal1.getPrecision());
        Assert.assertEquals(decimal1.getScale(), retDecimal1.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal1.getValue()).compareTo(new BigDecimal(retDecimal1.getValue())));
        System.out.println("2");
        System.out.println("decimal: " + decimal2);
        System.out.println("retDecimal: " + retDecimal2);
        System.out.println("bigDecimal: " + new BigDecimal(decimal2.getValue()));
        Assert.assertEquals(decimal2.getPrecision(), retDecimal2.getPrecision());
        Assert.assertEquals(decimal2.getScale(), retDecimal2.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal2.getValue()).compareTo(new BigDecimal(retDecimal2.getValue())));
        System.out.println("3");
        System.out.println("decimal: " + decimal3);
        System.out.println("retDecimal: " + retDecimal3);
        System.out.println("bigDecimal: " + new BigDecimal(decimal3.getValue()));

        Assert.assertEquals(decimal3.getPrecision(), retDecimal3.getPrecision());
        Assert.assertEquals(decimal3.getScale(), retDecimal3.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal3.getValue()).compareTo(new BigDecimal(retDecimal3.getValue())));

        System.out.println("finish decimalWithIntegerPartTest");
    }

    /**
     * 测试插入不带整数部分的小数，如：".123",".0123e5"
     */
    @Test
//	@Ignore
    public void decimalWithoutIntegerPartTest() {
        // case 1:
        BSONDecimal decimal1 = DecimalCommon.genBSONDecimal(false, false, true, rand.nextInt(1000));
        BSONDecimal decimal2 = DecimalCommon.genBSONDecimal(false, false, true, -rand.nextInt(1000));
        BSONDecimal decimal3 = DecimalCommon.genBSONDecimal(false, false, false, 0);
        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);

        BSONObject obj =
            new BasicBSONObject("case1", "test_in_java").append("f1", decimal1).
                append("f2", decimal2).append("f3", decimal3);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("f1");
        BSONDecimal retDecimal2 = (BSONDecimal) obj.get("f2");
        BSONDecimal retDecimal3 = (BSONDecimal) obj.get("f3");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);

        System.out.println("1");
        System.out.println("decimal: " + decimal1);
        System.out.println("retDecimal: " + retDecimal1);
        System.out.println("bigDecimal: " + new BigDecimal(decimal1.getValue()));
        Assert.assertEquals(decimal1.getPrecision(), retDecimal1.getPrecision());
        Assert.assertEquals(decimal1.getScale(), retDecimal1.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal1.getValue()).compareTo(new BigDecimal(retDecimal1.getValue())));
        System.out.println("2");
        System.out.println("decimal: " + decimal2);
        System.out.println("retDecimal: " + retDecimal2);
        System.out.println("bigDecimal: " + new BigDecimal(decimal2.getValue()));
        Assert.assertEquals(decimal2.getPrecision(), retDecimal2.getPrecision());
        Assert.assertEquals(decimal2.getScale(), retDecimal2.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal2.getValue()).compareTo(new BigDecimal(retDecimal2.getValue())));
        System.out.println("3");
        System.out.println("decimal: " + decimal3);
        System.out.println("retDecimal: " + retDecimal3);
        System.out.println("bigDecimal: " + new BigDecimal(decimal3.getValue()));

        Assert.assertEquals(decimal3.getPrecision(), retDecimal3.getPrecision());
        Assert.assertEquals(decimal3.getScale(), retDecimal3.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal3.getValue()).compareTo(new BigDecimal(retDecimal3.getValue())));

        System.out.println("finish decimalWithoutIntegerPartTest");
    }

    /**
     * 测试四舍五入正确性
     */
    @Test
//	@Ignore
    public void roundingTest() {
        // case 1: positive
        String str = "1234.0678998765432198765";
        String expectStr1 = "1234.1"; // $precision is: [5, 1]
        String expectStr2 = "1234.0679"; // $precision is: [8, 4]
        String expectStr3 = "1234.06790"; // $precision is: [9, 5]
        String expectStr4 = "1234.067899876543219877"; // $precision is: [22, 18]

        BSONDecimal decimal1 = new BSONDecimal(str, 5, 1);
        BSONDecimal decimal2 = new BSONDecimal(str, 8, 4);
        BSONDecimal decimal3 = new BSONDecimal(str, 9, 5);
        BSONDecimal decimal4 = new BSONDecimal(str, 22, 18);

        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);
        System.out.println("decimal4 is: " + decimal4);

        BSONObject obj =
            new BasicBSONObject("case1", "test_in_java").append("f1", decimal1).
                append("f2", decimal2).
                append("f3", decimal3).append("f4", decimal4);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("f1");
        BSONDecimal retDecimal2 = (BSONDecimal) obj.get("f2");
        BSONDecimal retDecimal3 = (BSONDecimal) obj.get("f3");
        BSONDecimal retDecimal4 = (BSONDecimal) obj.get("f4");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);
        System.out.println("retDecimal4 is: " + retDecimal4);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr2).compareTo(new BigDecimal(retDecimal2.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr3).compareTo(new BigDecimal(retDecimal3.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr4).compareTo(new BigDecimal(retDecimal4.getValue())));

        System.out.println("finish case 1 in roundingTest");

        // case 2: negative
        str = "-1234.0678998765432198765";
        expectStr1 = "-1234.1"; // $precision is: [5, 1]
        expectStr2 = "-1234.0679"; // $precision is: [8, 4]
        expectStr3 = "-1234.06790"; // $precision is: [9, 5]
        expectStr4 = "-1234.067899876543219877"; // $precision is: [22, 18]

        decimal1 = new BSONDecimal(str, 5, 1);
        decimal2 = new BSONDecimal(str, 8, 4);
        decimal3 = new BSONDecimal(str, 9, 5);
        decimal4 = new BSONDecimal(str, 22, 18);

        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);
        System.out.println("decimal4 is: " + decimal4);

        obj = new BasicBSONObject("case2", "test_in_java").append("f1", decimal1).
            append("f2", decimal2).
            append("f3", decimal3).append("f4", decimal4);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case2", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        retDecimal1 = (BSONDecimal) obj.get("f1");
        retDecimal2 = (BSONDecimal) obj.get("f2");
        retDecimal3 = (BSONDecimal) obj.get("f3");
        retDecimal4 = (BSONDecimal) obj.get("f4");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);
        System.out.println("retDecimal4 is: " + retDecimal4);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr2).compareTo(new BigDecimal(retDecimal2.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr3).compareTo(new BigDecimal(retDecimal3.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr4).compareTo(new BigDecimal(retDecimal4.getValue())));

        System.out.println("finish case 2 in roundingTest");

        // case 3: positive exponent
        str = "999999999999.9999996789012345E-5";
        expectStr1 = "9999999.999999999997"; // $precision is: [19, 12]
        expectStr2 = "9999999.99999999999678901235"; // $precision is: [27, 20]
        expectStr3 = "10000000.00000000000"; // $precision is: [19, 11]
        expectStr4 = "10000000.0"; // $precision is: [13, 1]

        decimal1 = new BSONDecimal(str, 19, 12);
        decimal2 = new BSONDecimal(str, 27, 20);
        decimal3 = new BSONDecimal(str, 19, 11);
        decimal4 = new BSONDecimal(str, 13, 1);

        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);
        System.out.println("decimal4 is: " + decimal4);

        obj = new BasicBSONObject("case3", "test_in_java").append("f1", decimal1).
            append("f2", decimal2).
            append("f3", decimal3).append("f4", decimal4);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case3", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        retDecimal1 = (BSONDecimal) obj.get("f1");
        retDecimal2 = (BSONDecimal) obj.get("f2");
        retDecimal3 = (BSONDecimal) obj.get("f3");
        retDecimal4 = (BSONDecimal) obj.get("f4");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);
        System.out.println("retDecimal4 is: " + retDecimal4);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr2).compareTo(new BigDecimal(retDecimal2.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr3).compareTo(new BigDecimal(retDecimal3.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr4).compareTo(new BigDecimal(retDecimal4.getValue())));

        System.out.println("finish case 3 in roundingTest");

        // case 4: positive exponent
        str = "-99999999.99999999996789012345E5";
        expectStr1 = "-9999999999999.999997"; // $precision is: [19, 6]
        expectStr2 = "-9999999999999.99999678901235"; // $precision is: [27, 14]
        expectStr3 = "-10000000000000.00000"; // $precision is: [19, 5]
        expectStr4 = "-10000000000000.0"; // $precision is: [15, 1]

        decimal1 = new BSONDecimal(str, 19, 6);
        decimal2 = new BSONDecimal(str, 27, 14);
        decimal3 = new BSONDecimal(str, 19, 5);
        decimal4 = new BSONDecimal(str, 15, 1);

        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);
        System.out.println("decimal4 is: " + decimal4);

        obj = new BasicBSONObject("case4", "test_in_java").append("f1", decimal1).
            append("f2", decimal2).
            append("f3", decimal3).append("f4", decimal4);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case4", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        retDecimal1 = (BSONDecimal) obj.get("f1");
        retDecimal2 = (BSONDecimal) obj.get("f2");
        retDecimal3 = (BSONDecimal) obj.get("f3");
        retDecimal4 = (BSONDecimal) obj.get("f4");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);
        System.out.println("retDecimal4 is: " + retDecimal4);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr2).compareTo(new BigDecimal(retDecimal2.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr3).compareTo(new BigDecimal(retDecimal3.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr4).compareTo(new BigDecimal(retDecimal4.getValue())));

        System.out.println("finish case 4 in roundingTest");

    }

    @Test
//	@Ignore
    public void roundingTest2() {
        // case 1: positive
        String str = "9999.99999";
        String expectStr1 = "10000.0000"; // $precision is: [9, 4]

        BSONDecimal decimal1 = new BSONDecimal(str, 9, 4);

        System.out.println("decimal1 is: " + decimal1);

        BSONObject obj =
            new BasicBSONObject("case1", "test_in_java").append("f1", decimal1);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("f1");

        System.out.println("retDecimal1 is: " + retDecimal1);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        System.out.println("finish case 1 in roundingTest2");
    }

    @Test
//	@Ignore
    public void roundingTes3() {
        // case 1: positive
        String str = "9999.96";
        String expectStr1 = "10000.0"; // $precision is: [9, 4]

        BSONDecimal decimal1 = new BSONDecimal(str, 6, 1);

        System.out.println("decimal1 is: " + decimal1);

        BSONObject obj =
            new BasicBSONObject("case1", "test_in_java").append("f1", decimal1);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("f1");

        System.out.println("retDecimal1 is: " + retDecimal1);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        System.out.println("finish case 1 in roundingTest3");
    }

    @Test
//	@Ignore
    public void roundingTes4() {
        // case 1: positive
        String str = "0.00006";
        String str2 = "1234.00006";
        String expectStr1 = "0.0001"; // $precision is: [4, 4]
        String expectStr2 = "1234.0001"; // $precision is: [8, 4]

        BSONDecimal decimal1 = new BSONDecimal(str, 4, 4);
        BSONDecimal decimal2 = new BSONDecimal(str2, 8, 4);

        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);

        BSONObject obj =
            new BasicBSONObject("case1", "test_in_java").append("f1", decimal1).append("f2", decimal2);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("f1");
        BSONDecimal retDecimal2 = (BSONDecimal) obj.get("f2");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr2).compareTo(new BigDecimal(retDecimal2.getValue())));
        System.out.println("finish case 1 in roundingTest4");
    }

    @Test
//	@Ignore
    public void roundingTes5() {
        // case 1:
        String str = "9999.99999";
        String str2 = "9999.99999";
        String expectStr1 = "10000.0000"; // $precision is: [10, 4]
        String expectStr2 = "10000.0000"; // $precision is: [10, 4]

        BSONDecimal decimal1 = new BSONDecimal(str, 10, 4);
        BSONDecimal decimal2 = new BSONDecimal(str2, 10, 4);

        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);

        BSONObject obj =
            new BasicBSONObject("case1", "test_in_java").append("f1", decimal1).append("f2", decimal2);
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONDecimal retDecimal1 = (BSONDecimal) obj.get("f1");
        BSONDecimal retDecimal2 = (BSONDecimal) obj.get("f2");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        // check
        Assert.assertEquals(0, new BigDecimal(expectStr1).compareTo(new BigDecimal(retDecimal1.getValue())));
        Assert.assertEquals(0, new BigDecimal(expectStr2).compareTo(new BigDecimal(retDecimal2.getValue())));
        System.out.println("finish case 1 in roundingTest5");
    }

    /**
     * 测试使用科学计数法
     */
    @Test
//	@Ignore
    public void scientificNotationTest() {
        // try{cl.truncate();}catch(Exception e){}
        String str1 = "1.23456E5";
        String str2 = "1.23456E+5";
        String str3 = "12345600000E-5";
        String str4 = "1.23456e5";
        String str5 = "1.23456e+5";
        String str6 = "12345600000e-5";

        String expectStr = "123456";

        BSONDecimal decimal1 = new BSONDecimal(str1);
        BSONDecimal decimal2 = new BSONDecimal(str2);
        BSONDecimal decimal3 = new BSONDecimal(str3);
        BSONDecimal decimal4 = new BSONDecimal(str4);
        BSONDecimal decimal5 = new BSONDecimal(str5);
        BSONDecimal decimal6 = new BSONDecimal(str6);

        BSONObject obj1 = new BasicBSONObject("case1", "test_in_java").append("f1", decimal1);
        BSONObject obj2 = new BasicBSONObject("case2", "test_in_java").append("f2", decimal2);
        BSONObject obj3 = new BasicBSONObject("case3", "test_in_java").append("f3", decimal3);
        BSONObject obj4 = new BasicBSONObject("case4", "test_in_java").append("f4", decimal4);
        BSONObject obj5 = new BasicBSONObject("case5", "test_in_java").append("f5", decimal5);
        BSONObject obj6 = new BasicBSONObject("case6", "test_in_java").append("f6", decimal6);

        System.out.println("inserted obj is: " + obj1);

        cl.insert(obj1);
        cl.insert(obj2);
        cl.insert(obj3);
        cl.insert(obj4);
        cl.insert(obj5);
        cl.insert(obj6);
        // case 1
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj1 = cur.getNext();
        System.out.println("returned obj is: " + obj1);
        Assert.assertEquals(expectStr, ((BSONDecimal) obj1.get("f1")).getValue());

        // case 2
        cur = cl.query(new BasicBSONObject().append("case2", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj1 = cur.getNext();
        System.out.println("returned obj is: " + obj1);
        Assert.assertEquals(expectStr, ((BSONDecimal) obj1.get("f2")).getValue());

        // case 3
        cur = cl.query(new BasicBSONObject().append("case3", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj1 = cur.getNext();
        System.out.println("returned obj is: " + obj1);
        BigDecimal big = new BigDecimal(expectStr);
        BigDecimal big2 = new BigDecimal(((BSONDecimal) obj1.get("f3")).getValue());
        Assert.assertEquals(0, big.compareTo(big2));

        // case 4
        cur = cl.query(new BasicBSONObject().append("case4", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj1 = cur.getNext();
        System.out.println("returned obj is: " + obj1);
        big = new BigDecimal(expectStr);
        big2 = new BigDecimal(((BSONDecimal) obj1.get("f4")).getValue());
        Assert.assertEquals(0, big.compareTo(big2));

        // case 5
        cur = cl.query(new BasicBSONObject().append("case5", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj1 = cur.getNext();
        System.out.println("returned obj is: " + obj1);
        big = new BigDecimal(expectStr);
        big2 = new BigDecimal(((BSONDecimal) obj1.get("f5")).getValue());
        Assert.assertEquals(0, big.compareTo(big2));

        // case 6
        cur = cl.query(new BasicBSONObject().append("case6", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj1 = cur.getNext();
        System.out.println("returned obj is: " + obj1);
        big = new BigDecimal(expectStr);
        big2 = new BigDecimal(((BSONDecimal) obj1.get("f6")).getValue());
        Assert.assertEquals(0, big.compareTo(big2));
    }


    /**
     * 测试Nan/Max/Min/Max Precision/Max Scale
     */
    @Test
    public void boundaryTest() {
        String MAX = "MAX";
        String MIN = "MIN";
        String NaN = "NaN";
        DBCursor cur = null;
        BSONObject obj = null;
        BSONObject retObj = null;
        BSONDecimal retDecimal = null;
        String str = null;
        String integer_str = null;
        String decimal_str = null;
        int maxPrecision = 0;
        int maxScale = 0;
        Random rand = new Random();

        // case 1: Max
        obj = new BasicBSONObject("case1", new BSONDecimal("max", 10, 5));
        System.out.println("insert max key record is： " + obj);
        cl.insert(obj);
        cur = cl.query(obj, null, null, null);
        Assert.assertTrue(cur.hasNext());
        retObj = cur.getNext();
        System.out.println("queried record is: " + retObj);
        retDecimal = (BSONDecimal) retObj.get("case1");
        System.out.println("value is: " + retDecimal.getValue());
        System.out.println("precision is: " + retDecimal.getPrecision());
        System.out.println("scale is: " + retDecimal.getScale());
        Assert.assertEquals(MAX, retDecimal.getValue());
        Assert.assertEquals(-1, retDecimal.getPrecision());
        Assert.assertEquals(-1, retDecimal.getScale());
        System.out.println("finish case 1");

        // case 2: Min
        obj = new BasicBSONObject("case2", new BSONDecimal("MIN", 10, 5));
        System.out.println("insert min record is： " + obj);
        cl.insert(obj);
        cur = cl.query(obj, null, null, null);
        Assert.assertTrue(cur.hasNext());
        retObj = cur.getNext();
        System.out.println("queried record is: " + retObj);
        retDecimal = (BSONDecimal) retObj.get("case2");
        System.out.println("value is: " + retDecimal.getValue());
        System.out.println("precision is: " + retDecimal.getPrecision());
        System.out.println("scale is: " + retDecimal.getScale());
        Assert.assertEquals(MIN, retDecimal.getValue());
        Assert.assertEquals(-1, retDecimal.getPrecision());
        Assert.assertEquals(-1, retDecimal.getScale());
        System.out.println("finish case 2");

        // case 3: Nan
        obj = new BasicBSONObject("case3", new BSONDecimal("Nan", 10, 5));
        System.out.println("insert nan record is： " + obj);
        cl.insert(obj);
        cur = cl.query(obj, null, null, null);
        Assert.assertTrue(cur.hasNext());
        retObj = cur.getNext();
        System.out.println("queried record is: " + retObj);
        retDecimal = (BSONDecimal) retObj.get("case3");
        System.out.println("value is: " + retDecimal.getValue());
        System.out.println("precision is: " + retDecimal.getPrecision());
        System.out.println("scale is: " + retDecimal.getScale());
        Assert.assertEquals(NaN, retDecimal.getValue());
        Assert.assertEquals(-1, retDecimal.getPrecision());
        Assert.assertEquals(-1, retDecimal.getScale());
        System.out.println("finish case 3");

        // case 4: Max Precision
        maxPrecision = 131072;
        integer_str = "9";
        for (int i = 1; i < maxPrecision; i++) {
            integer_str += rand.nextInt(10);
        }
        obj = new BasicBSONObject("case4", new BSONDecimal(integer_str));
        System.out.println("insert max precision record is： " + obj);
        cl.insert(obj);
        cur = cl.query(obj, null, null, null);
        Assert.assertTrue(cur.hasNext());
        retObj = cur.getNext();
        System.out.println("queried record is: " + retObj);
        retDecimal = (BSONDecimal) retObj.get("case4");
        System.out.println("precision is: " + retDecimal.getScale());
        Assert.assertEquals(integer_str, retDecimal.getValue());
        Assert.assertEquals(-1, retDecimal.getPrecision());
        Assert.assertEquals(-1, retDecimal.getScale());
        System.out.println("finish case 4");

        // case 5: Max Precision, but has strip
        str = "0000000000" + integer_str;
        try {
            obj = new BasicBSONObject("case5", new BSONDecimal(str));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
        System.out.println("finish case 5");

        // case 6: more than max precision(no strip)
        str = integer_str + rand.nextInt(10);
        try {
            obj = new BasicBSONObject("case6", new BSONDecimal(str));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
        System.out.println("finish case 6");

        // case 7: more than max precision(with strip)
        str = "0000000000" + integer_str + rand.nextInt(10);
        try {
            obj = new BasicBSONObject("case7", new BSONDecimal(str));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
        System.out.println("finish case 7");

        str = integer_str + "e1";
        try {
            obj = new BasicBSONObject("case100", new BSONDecimal(str));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
        System.out.println("finish case 100");

        // case 8: Max Scale
        maxScale = 16383;
        str = "0.";
        decimal_str = "56";
        for (int i = 2; i < maxScale; i++) {
            decimal_str += rand.nextInt(10);
        }
        str = str + decimal_str;
        obj = new BasicBSONObject("case8", new BSONDecimal(str));
        System.out.println("insert max scale record is： " + obj);
        cl.insert(obj);
        cur = cl.query(obj, null, null, null);
        Assert.assertTrue(cur.hasNext());
        retObj = cur.getNext();
        System.out.println("queried record is: " + retObj);
        retDecimal = (BSONDecimal) retObj.get("case8");
        System.out.println("precision is: " + retDecimal.getScale());
        Assert.assertEquals(str, retDecimal.getValue());
        Assert.assertEquals(-1, retDecimal.getPrecision());
        Assert.assertEquals(-1, retDecimal.getScale());
        System.out.println("finish case 8");

        // case 9: more than max scale(has round)
        str = str + "123456789";
        try {
            obj = new BasicBSONObject("case9", new BSONDecimal(str, 10, 1));
            Assert.fail();
            System.out.println("insert more max precision(has round) record is： " + obj);
            cl.insert(obj);
            cur = cl.query(obj, null, null, null);
            Assert.assertTrue(cur.hasNext());
            retObj = cur.getNext();
            System.out.println("queried record is: " + retObj);
            retDecimal = (BSONDecimal) retObj.get("case9");
            System.out.println("precision is: " + retDecimal.getScale());
            System.out.println("retDecimal is: " + retDecimal.getValue());
            Assert.assertEquals("0.6", retDecimal.getValue());
            Assert.assertEquals(10, retDecimal.getPrecision());
            Assert.assertEquals(1, retDecimal.getScale());
            System.out.println("finish case 9");
        } catch (IllegalArgumentException e) {
        }

        // case 10: more than max scale(no round)
        try {
            obj = new BasicBSONObject("case10", new BSONDecimal(str));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }

        // case 11: more than max scale(with scientific notation)
        try {
            str = str + "e-1";
            obj = new BasicBSONObject("case11", new BSONDecimal(str));
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }

    }

    /**
     * 测试Nan/Max/Min/Max Precision/Max Scale
     */
    @Test
    @Ignore
    public void boundaryTest2() {
//		DBCursor cur = null;
//		BSONObject obj = null;
//		BSONObject retObj = null;
//		BSONDecimal decimal = null;
//		BSONDecimal retDecimal = null;
//		String str = null;
//		long maxPrecision = 0;
//		long maxScale = 0;
//		int precision = 0;
//		int scale = 0;
//		Random rand = new Random();
//		
//		// case 1: precision is more than Integer.MAX_VALUE
//		maxPrecision = Integer.MAX_VALUE + 1;
//		str = "9";
//		for (int i = 1; i < maxPrecision; i++) {
//			str += rand.nextInt(10);
//		}
//		try {
//			obj = new BasicBSONObject("case1", new BSONDecimal(str));
//			Assert.fail();
//		} catch(IllegalArgumentException e) {
//		}

//		// case 2:
//		maxPrecision = Integer.MAX_VALUE + 1;
//		str = "9";
//		for (int i = 1; i < maxPrecision; i++) {
//			str += rand.nextInt(10);
//		}
//		String tmp = str + rand.nextInt(10);
//		obj = new BasicBSONObject("case4", new BSONDecimal(tmp));
//		System.out.println("insert max precision record is： " + obj);
//		cl.insert(obj);
//		cur = cl.query(obj, null, null, null);
//		Assert.assertTrue(cur.hasNext());
//		retObj = cur.getNext();
//		System.out.println("queried record is: " + retObj);
//		retDecimal = (BSONDecimal) retObj.get("case4");
//		System.out.println("precision is: " + retDecimal.getScale());
//		Assert.assertEquals(tmp, retDecimal.getValue());
//		Assert.assertEquals(-1, retDecimal.getPrecision());
//		Assert.assertEquals(-1, retDecimal.getScale());
//		System.out.println("finish case 4");
    }

    /**
     * 测试使用错误数据格式
     */
    @Test
    public void errorFormatTest() {
        ArrayList<String> strArr = new ArrayList<String>();
        strArr.add("12 345");
        strArr.add("12345. 345");
        strArr.add(". 1234");
        strArr.add("- 1.34");
        strArr.add("- .345");
        strArr.add("+-123.5");
        strArr.add("123.556E");
        strArr.add("23424. 4E");
        strArr.add("23424. 4E 5");
        strArr.add("23424. 4E +5");
        strArr.add("23424. 4E+ 5");
        strArr.add("23424. 4E -5");
        strArr.add("23424. 4E- 5");
        strArr.add("23425.3E 19");
        strArr.add("234a5");
        strArr.add("234E");
        strArr.add("e10");

        int precision = 0;
        int scale = 0;

        // case 1: invalid digits
        for (int i = 0; i < strArr.size(); i++) {
            String str = strArr.get(i);
            try {
                cl.insert(new BasicBSONObject("a", new BSONDecimal(str)));
                Assert.fail();
            } catch (IllegalArgumentException e) {
                //e.printStackTrace();
            }
        }
    }

    @Test
    public void errorFormatTest2() {
        String str = "123456.7890987";
        int precision = 0;
        int scale = 0;
        // precision [1, 1000]
        // scale [0, precision)
        ArrayList<DecimalPair> arr = new ArrayList<DecimalPair>();
        arr.add(new DecimalPair(-2, -2));
        arr.add(new DecimalPair(-1, -2));
        arr.add(new DecimalPair(-1, 0));
        arr.add(new DecimalPair(-1, 1));
        arr.add(new DecimalPair(0, -1));
        arr.add(new DecimalPair(0, 0));
        arr.add(new DecimalPair(0, 1));
        arr.add(new DecimalPair(1001, 1));
        arr.add(new DecimalPair(1000, 1000));
        arr.add(new DecimalPair(1000, 10001));

        // case 2: invalid precision and scale
        for (int i = 0; i < arr.size(); i++) {
            precision = arr.get(i).precision;
            scale = arr.get(i).scale;
            try {
                cl.insert(new BasicBSONObject("a", new BSONDecimal(str, precision, scale)));
                Assert.fail();
            } catch (IllegalArgumentException e) {
//				e.printStackTrace();
            }
        }
    }

    @Test
    public void errorFormatTest3() {
        ArrayList<String> strArr = new ArrayList<String>();
        strArr.add(null);
        strArr.add("");

        // case 1: invalid digits
        for (int i = 0; i < strArr.size(); i++) {
            String str = strArr.get(i);
            try {
                cl.insert(new BasicBSONObject("a", new BSONDecimal(str)));
                Assert.fail();
            } catch (IllegalArgumentException e) {
            }
        }
    }

    @Test
    public void errorFormatTest4() {
        try {
            BSONDecimal d = new BSONDecimal("12332.456", 10, 9);
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
    }

}
