package com.sequoiadb.decimal;

import java.math.BigDecimal;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Decimal9590.java TestLink:
 * seqDB-9590
 * 
 * @author zhaoyu
 * @Date 2016.9.28
 * @version 1.00
 */
public class Decimal9590 extends SdbTestBase {
    private Sequoiadb sdb;

    private CollectionSpace cs = null;
    private String clName = "cl9590";
    private DBCollection cl = null;
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            sdb = new Sequoiadb(coordAddr, "", "");
            if (!sdb.isCollectionSpaceExist(commCSName)) {
                sdb.createCollectionSpace(commCSName);
            }
            cs = sdb.getCollectionSpace(commCSName);
            if (cs.isCollectionExist(clName)) {
                cs.dropCollection(clName);
            }
            cl = cs.createCollection(clName);
        } catch (BaseException e) {
            Assert.fail("prepare env failed" + e.getMessage());
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if (cs.isCollectionExist(clName)) {
                cs.dropCollection(clName);
            }
            sdb.disconnect();
        } catch (BaseException e) {
            Assert.fail("clear env failed, errMsg:" + e.getMessage());
        }
    }

    @Test
    public void testBigDecimalToDecimal() {
        BSONObject obj = null;
        try {
            String value = "123456.123456789";
            String expectNoPrecision = "{\"$decimal\":\"123456.123456789\"}";

            obj = new BasicBSONObject();
            obj.put("a", new BigDecimal(value));
            cl.insert(obj);

            String actualNoPrecision = cl.queryOne().get("a").toString().replace(" ", "");
            Assert.assertEquals(actualNoPrecision, expectNoPrecision);

            cl.delete("{a:{$exists:1}}");
        } catch (IllegalArgumentException e) {
            Assert.fail("generate data:" + obj + " failed, errMsg:" + e.getMessage());
        } catch (BaseException e) {
            Assert.fail("insert data:" + obj + " failed, errMsg:" + e.getMessage());
        }
    }

    @Test
    public void testStringToDecimal() {
        BSONObject obj = null;
        try {
            String value = "123456.123456789";
            String expectNoPrecision = "{\"$decimal\":\"123456.123456789\"}";

            obj = new BasicBSONObject();
            BSONDecimal decimal = new BSONDecimal(value);
            obj.put("a", decimal);
            cl.insert(obj);

            String actualNoPrecision = cl.queryOne().get("a").toString().replace(" ", "");
            Assert.assertEquals(actualNoPrecision, expectNoPrecision);

            cl.delete("{a:{$exists:1}}");
        } catch (IllegalArgumentException e) {
            Assert.fail("generate data:" + obj + " failed, errMsg:" + e.getMessage());
        } catch (BaseException e) {
            Assert.fail("insert data:" + obj + " failed, errMsg:" + e.getMessage());
        }
    }

    @Test
    public void testDecimalWithPrecision() {
        BSONObject obj = null;
        try {
            String value = "123456.123456789";
            int precision = 20;
            int scale = 10;
            String expectWithPrecsion = "{\"$decimal\":\"123456.1234567890\",\"$precision\":[20,10]}";
            obj = new BasicBSONObject();
            BSONDecimal decimal = new BSONDecimal(value, precision, scale);
            obj.put("a", decimal);
            cl.insert(obj);
            String actualWithPrecision = cl.queryOne().get("a").toString().replace(" ", "");
            Assert.assertEquals(actualWithPrecision, expectWithPrecsion);

            cl.delete("{a:{$exists:1}}");
        } catch (IllegalArgumentException e) {
            Assert.fail("generate data:" + obj + " failed, errMsg:" + e.getMessage());
        } catch (BaseException e) {
            Assert.fail("insert data:" + obj + " failed, errMsg:" + e.getMessage());
        }
    }
}
