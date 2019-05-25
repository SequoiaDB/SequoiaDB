package com.sequoiadb.test.bson;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.junit.*;
import org.junit.runners.MethodSorters;

import java.util.Date;
import java.util.regex.Pattern;

@FixMethodOrder(MethodSorters.DEFAULT)
public class BSONNumberLongTest {

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

    /*
     * 测试插入查询int类型的数值的情况
     */
    private void func1(String func, String expect) {
        BSONObject result = null;
        BSONObject obj = new BasicBSONObject();
        obj.put("a", 0);
        obj.put("b", Integer.MAX_VALUE);
        obj.put("c", Integer.MIN_VALUE);
        cl.insert(obj);
        cursor = cl.query(new BasicBSONObject(), new BasicBSONObject("_id", new BasicBSONObject("$include", 0)), null, null);
        try {
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        System.out.println(func + "'s result is: " + result);
        Assert.assertEquals(expect, result.toString());
        Assert.assertEquals("0", result.get("a").toString());
        Assert.assertEquals(Integer.MAX_VALUE + "", result.get("b").toString());
        Assert.assertEquals(Integer.MIN_VALUE + "", result.get("c").toString());
    }

    @Test
    public void digits_is_int_test() {
        BSON.setJSCompatibility(false);
        String expect = "{ \"a\" : 0 , \"b\" : 2147483647 , \"c\" : -2147483648 }";
        func1("digits_is_int_test_case1", expect);
        BSON.setJSCompatibility(true);
        func1("digits_is_int_test_case2", expect);
    }

    /*
     * 测试插入查询大于Integer.MAX小于 （2^53 - 1）的long类型的情况
     */
    private void func2(String func, String expect) {
        BSONObject result = null;
        BSONObject obj = new BasicBSONObject();
        obj.put("a", 0);
        obj.put("b", Integer.MAX_VALUE + 1L);
        obj.put("c", Integer.MIN_VALUE - 1L);
        cl.insert(obj);
        cursor = cl.query(new BasicBSONObject(), new BasicBSONObject("_id", new BasicBSONObject("$include", 0)), null, null);
        try {
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        System.out.println(func + "'s result is: " + result);
        Assert.assertEquals(expect, result.toString());
        Assert.assertEquals("0", result.get("a").toString());
        Assert.assertEquals((Integer.MAX_VALUE + 1L) + "", result.get("b").toString());
        Assert.assertEquals((Integer.MIN_VALUE - 1L) + "", result.get("c").toString());
    }

    @Test
    public void digits_is_long_test() {
        BSON.setJSCompatibility(false);
        String expect = "{ \"a\" : 0 , \"b\" : 2147483648 , \"c\" : -2147483649 }";
        func2("digits_is_long_test_case1", expect);
        BSON.setJSCompatibility(true);
        func2("digits_is_long_test_case2", expect);
    }

    /*
     * 测试插入查询大于 （2^53 - 1）小于Long.MAX的long类型的情况
     */
    private void func3(String func, String expect) {
        BSONObject result = null;
        BSONObject obj = new BasicBSONObject();
        obj.put("a", 0);
        obj.put("b", Long.MAX_VALUE);
        obj.put("c", Long.MIN_VALUE);
        cl.insert(obj);
        cursor = cl.query(new BasicBSONObject(), new BasicBSONObject("_id", new BasicBSONObject("$include", 0)), null, null);
        try {
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        System.out.println(func + "'s result is: " + result);
        Assert.assertEquals(expect, result.toString());
        Assert.assertEquals("0", result.get("a").toString());
        Assert.assertEquals(Long.MAX_VALUE + "", result.get("b").toString());
        Assert.assertEquals(Long.MIN_VALUE + "", result.get("c").toString());
    }

    @Test
    public void digits_is_long_test2() {
        BSON.setJSCompatibility(true);
        String expect = "{ \"a\" : 0 , \"b\" : { \"$numberLong\" : \"9223372036854775807\" } , \"c\" : { \"$numberLong\" : \"-9223372036854775808\" } }";
        func3("digits_is_long_test2_case2", expect);

        BSON.setJSCompatibility(false);
        String expect2 = "{ \"a\" : 0 , \"b\" : 9223372036854775807 , \"c\" : -9223372036854775808 }";
        func3("digits_is_long_test2_case1", expect2);

        BSON.setJSCompatibility(true);
        func3("digits_is_long_test2_case2", expect);
    }

    /*
     * 由于BSON.setJSCompatibility()是全局设置的，查看前一个设置是否会对后一个用例产生影响。
     * 注意：需要确保用例顺序执行，否则会产生随机错误。
     */
    @Test
    public void digits_is_long_test3() {
        Assert.assertTrue(BSON.getJSCompatibility());
        String expect = "{ \"a\" : 0 , \"b\" : { \"$numberLong\" : \"9223372036854775807\" } , \"c\" : { \"$numberLong\" : \"-9223372036854775808\" } }";
        func3("digits_is_long_test3_case1", expect);
    }


    @Test
    @Ignore
    public void tmp_test() {
        BSONObject result = null;
        Date date = new Date();
        BSONTimestamp timestamp = new BSONTimestamp(1000, 1000);
        Pattern rgx = Pattern.compile("^2001", Pattern.CASE_INSENSITIVE);
        BSONObject obj = new BasicBSONObject();
        obj.put("a", date);
        obj.put("b", timestamp);
        obj.put("c", rgx);
        cl.insert(obj);
        cursor = cl.query(new BasicBSONObject(), new BasicBSONObject("_id", new BasicBSONObject("$include", 0)), null, null);
        try {
            result = cursor.getNext();
        } finally {
            cursor.close();
        }

        System.out.println("tmp_test's result is: " + result);
        System.out.println("date is: " + date);
        System.out.println("timestamp is: " + timestamp);
        System.out.println("regex is: " + rgx);
    }

}
