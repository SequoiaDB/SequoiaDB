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
import org.bson.types.BasicBSONList;
import org.junit.*;

import java.math.BigDecimal;

import static org.junit.Assert.assertTrue;

/**
 * @author tanzhaobo
 * @brief 测试在嵌套情况下，BSONDecial类型的表现
 */
public class BSONTest {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cur;

    @BeforeClass
    public static void beforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
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

    /**
     * 将BSONDecimal嵌套于对象中
     */
    @Test
    public void nestingBSONDecimalInObjectTest() {
        BSONDecimal decimal1 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal2 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal3 = DecimalCommon.genBSONDecimal();
        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);

        BSONObject obj = null;
        BSONObject subObj2 = new BasicBSONObject("d", decimal2);
        BSONObject subObj1 = new BasicBSONObject("b", decimal1).append("c", subObj2);
        obj = new BasicBSONObject("a", subObj1).append("aa", decimal3).append("case1", "test_in_java");
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONObject objA = (BasicBSONObject) obj.get("a");
        BSONDecimal retDecimal1 = (BSONDecimal) objA.get("b");
        BSONObject objC = (BasicBSONObject) objA.get("c");
        BSONDecimal retDecimal2 = (BSONDecimal) objC.get("d");
        BSONDecimal retDecimal3 = (BSONDecimal) obj.get("aa");
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
     * 将BSONDecimal嵌套于数组中
     */
    @Test
    public void nestingBSONDecimalInArrayTest() {
        BSONDecimal decimal1 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal2 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal3 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal4 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal5 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal6 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal7 = DecimalCommon.genBSONDecimal();
        BSONDecimal decimal8 = DecimalCommon.genBSONDecimal();
        System.out.println("decimal1 is: " + decimal1);
        System.out.println("decimal2 is: " + decimal2);
        System.out.println("decimal3 is: " + decimal3);
        System.out.println("decimal4 is: " + decimal4);
        System.out.println("decimal5 is: " + decimal5);
        System.out.println("decimal6 is: " + decimal6);
        System.out.println("decimal7 is: " + decimal7);
        System.out.println("decimal8 is: " + decimal8);

        BSONObject obj = null;
        BSONObject subObj1 = new BasicBSONObject("b", decimal1);
        BSONObject subArr1 = new BasicBSONList();
        subArr1.put("0", decimal2);
        subArr1.put("1", decimal3);
        subArr1.put("2", decimal4);
        BSONObject subArr2 = new BasicBSONList();
        subArr2.put("0", decimal5);
        subArr2.put("1", decimal6);
        BSONObject subObj2 = new BasicBSONObject("e", decimal7);
        BSONObject subObj3 = new BasicBSONObject("f", decimal8);
        BSONObject subArr3 = new BasicBSONList();
        subArr3.put("0", subObj2);
        subArr3.put("1", subObj3);
        BSONObject subObj4 = new BasicBSONObject("c", subArr2).append("d", subArr3);

        obj = new BasicBSONObject("a", subObj1).append("aa", subArr1).append("aaa", subObj4).append("case1", "test_in_java");
        System.out.println("inserted obj is: " + obj);
        cl.insert(obj);
        cur = cl.query(new BasicBSONObject().append("case1", new BasicBSONObject("$exists", 1)),
            null, null, null);
        assertTrue(cur.hasNext());
        obj = cur.getNext();
        System.out.println("returned obj is: " + obj);
        BSONObject objA = (BasicBSONObject) obj.get("a");
        BSONDecimal retDecimal1 = (BSONDecimal) objA.get("b");

        BSONObject arrAA = (BasicBSONList) obj.get("aa");
        BSONDecimal retDecimal2 = (BSONDecimal) arrAA.get("0");
        BSONDecimal retDecimal3 = (BSONDecimal) arrAA.get("1");
        BSONDecimal retDecimal4 = (BSONDecimal) arrAA.get("2");

        BSONObject objAAA = (BasicBSONObject) obj.get("aaa");
        BSONObject arrC = (BasicBSONList) objAAA.get("c");
        BSONDecimal retDecimal5 = (BSONDecimal) arrC.get("0");
        BSONDecimal retDecimal6 = (BSONDecimal) arrC.get("1");

        BSONObject arrD = (BasicBSONList) objAAA.get("d");
        BSONObject objE = (BasicBSONObject) arrD.get("0");
        BSONObject objF = (BasicBSONObject) arrD.get("1");
        BSONDecimal retDecimal7 = (BSONDecimal) objE.get("e");
        BSONDecimal retDecimal8 = (BSONDecimal) objF.get("f");

        System.out.println("retDecimal1 is: " + retDecimal1);
        System.out.println("retDecimal2 is: " + retDecimal2);
        System.out.println("retDecimal3 is: " + retDecimal3);
        System.out.println("retDecimal4 is: " + retDecimal4);
        System.out.println("retDecimal5 is: " + retDecimal5);
        System.out.println("retDecimal6 is: " + retDecimal6);
        System.out.println("retDecimal7 is: " + retDecimal7);
        System.out.println("retDecimal8 is: " + retDecimal8);


        Assert.assertEquals(decimal1.getPrecision(), retDecimal1.getPrecision());
        Assert.assertEquals(decimal1.getScale(), retDecimal1.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal1.getValue()).compareTo(new BigDecimal(retDecimal1.getValue())));

        Assert.assertEquals(decimal2.getPrecision(), retDecimal2.getPrecision());
        Assert.assertEquals(decimal2.getScale(), retDecimal2.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal2.getValue()).compareTo(new BigDecimal(retDecimal2.getValue())));

        Assert.assertEquals(decimal3.getPrecision(), retDecimal3.getPrecision());
        Assert.assertEquals(decimal3.getScale(), retDecimal3.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal3.getValue()).compareTo(new BigDecimal(retDecimal3.getValue())));

        Assert.assertEquals(decimal4.getPrecision(), retDecimal4.getPrecision());
        Assert.assertEquals(decimal4.getScale(), retDecimal4.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal4.getValue()).compareTo(new BigDecimal(retDecimal4.getValue())));

        Assert.assertEquals(decimal5.getPrecision(), retDecimal5.getPrecision());
        Assert.assertEquals(decimal5.getScale(), retDecimal5.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal5.getValue()).compareTo(new BigDecimal(retDecimal5.getValue())));

        Assert.assertEquals(decimal6.getPrecision(), retDecimal6.getPrecision());
        Assert.assertEquals(decimal6.getScale(), retDecimal6.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal6.getValue()).compareTo(new BigDecimal(retDecimal6.getValue())));

        Assert.assertEquals(decimal7.getPrecision(), retDecimal7.getPrecision());
        Assert.assertEquals(decimal7.getScale(), retDecimal7.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal7.getValue()).compareTo(new BigDecimal(retDecimal7.getValue())));

        Assert.assertEquals(decimal8.getPrecision(), retDecimal8.getPrecision());
        Assert.assertEquals(decimal8.getScale(), retDecimal8.getScale());
        Assert.assertEquals(0, new BigDecimal(decimal8.getValue()).compareTo(new BigDecimal(retDecimal8.getValue())));

        System.out.println("finish");
    }


}
