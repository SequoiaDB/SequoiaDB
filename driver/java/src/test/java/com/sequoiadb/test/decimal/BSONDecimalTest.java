package com.sequoiadb.test.decimal;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.DecimalCommon;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.junit.*;
import org.junit.rules.ExpectedException;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.math.MathContext;

import static org.hamcrest.core.StringContains.containsString;
import static org.junit.Assert.assertTrue;

/**
 * @author tanzhaobo
 * @brief 测试对外的BSONDecimal类型的对外接口
 */
public class BSONDecimalTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cur;
    @Rule
    public ExpectedException thrown = ExpectedException.none();

    @BeforeClass
    public static void beforeClass() throws Exception {
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
    public void convertMAXValueToBigDecimalTest() {
        BSONDecimal bsonDecimal;
        BigDecimal bigDecimal;
        thrown.expect(UnsupportedOperationException.class);
        thrown.expectMessage(containsString("can't convert MAX to BigDecimal"));
        bsonDecimal = new BSONDecimal("MAX", 20, 10);
        bigDecimal = bsonDecimal.toBigDecimal();
    }

    @Test
    public void convertMINValueToBigDecimalTest() {
        BSONDecimal bsonDecimal;
        BigDecimal bigDecimal;
        thrown.expect(UnsupportedOperationException.class);
        thrown.expectMessage(containsString("can't convert MIN to BigDecimal"));

        bsonDecimal = new BSONDecimal("MIN", 20, 10);
        bigDecimal = bsonDecimal.toBigDecimal();
    }

    @Test
    public void convertNANValueToBigDecimalTest() {
        BSONDecimal bsonDecimal;
        BigDecimal bigDecimal;
        thrown.expect(UnsupportedOperationException.class);
        thrown.expectMessage(containsString("can't convert NaN to BigDecimal"));
        bsonDecimal = new BSONDecimal("NAN", 20, 10);
        bigDecimal = bsonDecimal.toBigDecimal();
    }

    /**
     * 用户构建BSONDecimal对象后，对象持有的内容是否正确。
     */
    @Test
    public void buildBSONDecimalTest() {
        BSONObject obj = null;
        BSONDecimal decimal = null;
        BSONDecimal retDecimal = null;
        String value = null;
        String retValue = null;
        String expectValue = null;
        int precision = 0;
        int retPrecision = 0;
        int scale = 0;
        int retScale = 0;

        // case 1: specify string value, precision, scale
        value = "123456789.0987654321";
        expectValue = "123456789.098765432100000";
        precision = 30;
        scale = 15;
        decimal = new BSONDecimal(value, precision, scale);
        obj = new BasicBSONObject().append("case1", decimal);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            new BasicBSONObject("case1", ""), null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        retDecimal = (BSONDecimal) obj.get("case1");
        retValue = retDecimal.getValue();
        retPrecision = retDecimal.getPrecision();
        retScale = retDecimal.getScale();
        Assert.assertEquals(expectValue, retValue);
        Assert.assertEquals(precision, retPrecision);
        Assert.assertEquals(scale, retScale);

        // case 2: specify string value
        value = "1.234567890987654321";
        expectValue = value;
        decimal = new BSONDecimal(value);
        obj = new BasicBSONObject().append("case2", decimal);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case2", new BasicBSONObject("$exists", 1)),
            new BasicBSONObject("case2", ""), null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        retDecimal = (BSONDecimal) obj.get("case2");
        retValue = retDecimal.getValue();
        retPrecision = retDecimal.getPrecision();
        retScale = retDecimal.getScale();
        Assert.assertEquals(expectValue, retValue);
        Assert.assertEquals(-1, retPrecision);
        Assert.assertEquals(-1, retScale);

        // case 3: specify BigDecimal
        value = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        expectValue = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890.1234567890";
        MathContext context = new MathContext(75);

        BigInteger bigInt = new BigInteger(value);
        BigDecimal big = new BigDecimal(bigInt, 10, context);
        decimal = new BSONDecimal(big);
        Assert.assertEquals(-1, decimal.getPrecision());
        Assert.assertEquals(-1, decimal.getScale());
        obj = new BasicBSONObject().append("case3", decimal);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case3", new BasicBSONObject("$exists", 1)),
            new BasicBSONObject("case3", ""), null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        retDecimal = (BSONDecimal) obj.get("case3");
        retValue = retDecimal.getValue();
        retPrecision = retDecimal.getPrecision();
        retScale = retDecimal.getScale();
        Assert.assertEquals(-1, retPrecision);
        Assert.assertEquals(-1, retScale);

        // case 4: toBigDecimal
        BigDecimal big2 = retDecimal.toBigDecimal();
        Assert.assertEquals(0, big.compareTo(big2));

        // case 5: getValue
        Assert.assertEquals(big.toPlainString(), decimal.getValue());
        Assert.assertEquals(big.toPlainString(), retDecimal.getValue());

    }

    /**
     * 用户构建BSONDecimal对象后，对象持有的内容是否正确。
     */
    @Test
    public void BSONDecimalEqualsTest() {
        String str = null;
        String lhs = null;
        String rhs = null;
        BSONDecimal decimal = null;
        BSONDecimal decimal1 = null;
        BSONDecimal decimal2 = null;
        int precision = 0;
        int scale = 0;

        // case 1: no precision and scale
        decimal = DecimalCommon.genBSONDecimal(false, true, false, 0);
        str = decimal.getValue();
        decimal1 = new BSONDecimal(str);
        decimal2 = new BSONDecimal(str);
        Assert.assertTrue(decimal1.equals(decimal2));


        // case 2: have precision and scale
//		decimal = DecimalCommon.genBSONDecimal(true, true, false, 0);
        str = "123456789.1234567890123456789";
        decimal = new BSONDecimal(str, 30, 15);
        str = decimal.getValue();
        precision = decimal.getPrecision();
        scale = decimal.getScale();
        System.out.println("precision is: " + decimal);
        decimal1 = new BSONDecimal(str, precision, scale);
        decimal2 = new BSONDecimal(str, precision, scale);
        Assert.assertTrue(decimal1.equals(decimal2));

        // case 3: one has but another not
//		decimal = DecimalCommon.genBSONDecimal(true, true, false, 0);
        str = "123456789.1234567890123456789";
        decimal1 = new BSONDecimal(str, 100, 50);
        decimal2 = new BSONDecimal(str);
        decimal1 = new BSONDecimal(str, precision, scale);
        decimal2 = new BSONDecimal(str, precision, scale);
        Assert.assertTrue(decimal1.equals(decimal2));

        // case 4: compare with itself
        str = "123456789.1234567890123456789";
        decimal1 = new BSONDecimal(str, 100, 50);
        decimal2 = new BSONDecimal(str);
        decimal1 = new BSONDecimal(str, precision, scale);
        decimal2 = new BSONDecimal(str, precision, scale);
        Assert.assertTrue(decimal1.equals(decimal1));
        Assert.assertTrue(decimal2.equals(decimal2));

        // case 5: has the same value, but different expression
        lhs = "1.23456789E8";
        rhs = "123456789";
        decimal1 = new BSONDecimal(lhs);
        decimal2 = new BSONDecimal(rhs);
        Assert.assertTrue(decimal1.equals(decimal2));
        Assert.assertTrue(decimal1.equals(decimal1));
        Assert.assertTrue(decimal2.equals(decimal2));

        // case 6: not equal
        lhs = "1.23456789";
        rhs = "123456789";
        decimal1 = new BSONDecimal(lhs);
        decimal2 = new BSONDecimal(rhs);
        Assert.assertFalse(decimal1.equals(decimal2));

    }

    /**
     * 用户使用不同的精度构建BSONDecimal对象
     */
    @Test
    public void buildBSONDecimalTest2() {
        BSONObject obj = null;
        BSONDecimal decimal = null;

        // case 1: specify invalid scale
        String value = "12345.6789";
        try {
            decimal = new BSONDecimal(value, 1, -1);
            Assert.fail();
            obj = new BasicBSONObject().append("case1", decimal);
            System.out.println("inserted obj is: " + obj);
            cl.insert(obj);
        } catch (IllegalArgumentException e) {
            // ok
        } catch (Exception e) {
            Assert.fail();
        }
    }

    /**
     * 用户直接append/add BigDecimal对象
     */
    @Test
    public void buildBSONDecimalTest3() {
        String str1 = "1.23";
        String str2 = "3.14";
        BasicBSONObject ret = null;
        BSONObject obj = new BasicBSONObject("case1", "test_in_java");
        obj.put("a", new BigDecimal("1.23"));

        BasicBSONObject obj2 = new BasicBSONObject("case2", "test_in_java");
        obj2.append("a", new BigDecimal("3.14"));

        cl.insert(obj);
        cl.insert(obj2);

        cur = cl.query(new BasicBSONObject("case1", new BasicBSONObject("$exists", 1)),
            new BasicBSONObject("a", ""), null, null);
        ret = (BasicBSONObject) cur.getNext();
        BigDecimal bDecimal1 = ret.getBigDecimal("a");
        Assert.assertEquals(new BigDecimal(str1), bDecimal1);


        cur = cl.query(new BasicBSONObject("case2", new BasicBSONObject("$exists", 1)),
            new BasicBSONObject("a", ""), null, null);
        ret = (BasicBSONObject) cur.getNext();
        BigDecimal bDecimal2 = ret.getBigDecimal("a");
        Assert.assertEquals(new BigDecimal(str2), bDecimal2);
    }

    @Test
    public void buildBSONDecimalTest4() {
        String str1 = "0.6789";
        String str2 = "1.2345";
        BasicBSONObject ret = null;

        /// 使用decimal内容构建bson
        // 方式一（原始方式）：
        BasicBSONObject obj1 = new BasicBSONObject("case1", "test_in_java");
        obj1.put("field1", new BSONDecimal(str1));
        System.out.println("obj1 is: " + obj1); // 输出： { "field1" : { "$decimal" : "0.6789"}}
        // 方式二（新增方式）：
        BasicBSONObject obj2 = new BasicBSONObject("case2", "test_in_java");
        obj2.put("field2", new BigDecimal(str2));
        System.out.println("obj2 is: " + obj2); // 输出：{ "field2" : { "$decimal" : "1.2345"}}

        /// 从bson中获取decimal内容
        // 方式一（原始方式）：
        BSONDecimal bsonDecimal = (BSONDecimal) obj1.get("field1");
        BigDecimal decimal1 = bsonDecimal.toBigDecimal();
        System.out.println("decimal1 is: " + decimal1); // 输出：0.6789

        // 方式二（新增方式）：
        BigDecimal decimal3 = obj1.getBigDecimal("field1");
        System.out.println("decimal3 is: " + decimal3); // 输出：0.6789
        BigDecimal decimal4 = obj2.getBigDecimal("field2");
        System.out.println("decimal4 is: " + decimal4); // 输出：1.2345

        cl.insert(obj1);
        cl.insert(obj2);

        cur = cl.query(new BasicBSONObject("case1", new BasicBSONObject("$exists", 1)),
            new BasicBSONObject("field1", ""), null, null);
        ret = (BasicBSONObject) cur.getNext();
        BigDecimal bDecimal1 = ret.getBigDecimal("field1");
        Assert.assertEquals(new BigDecimal(str1), bDecimal1);

        cur = cl.query(new BasicBSONObject("case2", new BasicBSONObject("$exists", 1)),
            new BasicBSONObject("field2", ""), null, null);
        ret = (BasicBSONObject) cur.getNext();
        BigDecimal bDecimal2 = ret.getBigDecimal("field2");
        Assert.assertEquals(new BigDecimal(str2), bDecimal2);
    }

    @Test
    public void buildBSONDecimalSample() {

        /// 使用decimal内容构建bson
        // 方式一（原始方式）：
        BasicBSONObject obj1 = new BasicBSONObject();
        obj1.put("field1", new BSONDecimal("0.6789"));
        System.out.println("obj1 is: " + obj1); // 输出： { "field1" : { "$decimal" : "0.6789"}}
        // 方式二（新增方式）：
        BasicBSONObject obj2 = new BasicBSONObject();
        obj2.put("field2", new BigDecimal("1.2345"));
        System.out.println("obj2 is: " + obj2); // 输出：{ "field2" : { "$decimal" : "1.2345"}}

        /// 从bson中获取decimal内容
        // 方式一（原始方式）：
        BSONDecimal bsonDecimal = (BSONDecimal) obj1.get("field1");
        BigDecimal decimal1 = bsonDecimal.toBigDecimal();
        System.out.println("decimal1 is: " + decimal1); // 输出：0.6789

        // 方式二（新增方式）：
        BigDecimal decimal3 = obj1.getBigDecimal("field1");
        System.out.println("decimal3 is: " + decimal3); // 输出：0.6789
        BigDecimal decimal4 = obj2.getBigDecimal("field2");
        System.out.println("decimal4 is: " + decimal4); // 输出：1.2345

    }

    @Test
    public void bug_jira_1990_q1_q2_q3() {
        // TODO: test more
        String str = null;
        int precision = 0;
        int scale = 0;
        BSONDecimal d = null;

        // q1
        str = "112233.112233445566778899";
        precision = 21;
        scale = 18;
        try {
            d = new BSONDecimal(str, precision, scale);
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }

        // q2
        str = "123";
        precision = 6;
        scale = 4;
        try {
            d = new BSONDecimal(str, precision, scale);
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }

        // q3
        str = "1";
        precision = 1000;
        scale = 1000;
        try {
            d = new BSONDecimal(str, precision, scale);
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
        str = "123";
        precision = 5;
        scale = 5;
        try {
            d = new BSONDecimal(str, precision, scale);
            Assert.fail();
        } catch (IllegalArgumentException e) {
        }
    }

}
