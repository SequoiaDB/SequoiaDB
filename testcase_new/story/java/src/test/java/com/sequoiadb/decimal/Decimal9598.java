package com.sequoiadb.decimal;

import java.math.BigDecimal;
import java.util.Date;
import java.util.Map;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Decimal9598.java
 * TestLink: seqDB-9598
 * 
 * @author zhaoyu
 * @Date 2016.9.29
 * @version 1.00
 */

public class Decimal9598 {

    @BeforeClass
    public void setUp() {
        try {
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {

        BSONDecimal decimal1 = null;
        BSONDecimal decimal2 = null;
        BSONDecimal decimal3 = null;
        BSONDecimal actualDecimal1 = null;
        BSONDecimal actualDecimal2 = null;
        BSONDecimal actualDecimal3 = null;
        BasicBSONObject obj1 = new BasicBSONObject();
        BasicBSONObject obj2 = new BasicBSONObject();
        BasicBSONObject obj3 = new BasicBSONObject();

        try {
            String value1 = "1123456789.12345678901234";
            BigDecimal bigDecimal = new BigDecimal( value1 );
            decimal1 = new BSONDecimal( bigDecimal );
            obj1.put( "a", decimal1 );
            @SuppressWarnings("unchecked")
            Map< String, Object > map1 = obj1.toMap();
            actualDecimal1 = ( BSONDecimal ) map1.get( "a" );
            System.out.println( "map1:" + map1 );

            String value2 = "170E308";
            decimal2 = new BSONDecimal( value2 );
            obj2.put( "b", decimal2 );
            @SuppressWarnings("unchecked")
            Map< String, Object > map2 = obj2.toMap();
            actualDecimal2 = ( BSONDecimal ) map2.get( "b" );

            String value3 = "-49406.5645841246544e-350";
            int precision = 1000;
            int scale = 500;
            decimal3 = new BSONDecimal( value3, precision, scale );
            obj3.put( "c", decimal3 );
            @SuppressWarnings("unchecked")
            Map< String, Object > map3 = obj3.toMap();
            actualDecimal3 = ( BSONDecimal ) map3.get( "c" );

        } catch ( BaseException e ) {
            Assert.fail(
                    "generate BSONObject failed, errMsg:" + e.getMessage() );
        }

        Assert.assertEquals( actualDecimal1, decimal1 );
        Assert.assertEquals( actualDecimal2, decimal2 );
        Assert.assertEquals( actualDecimal3, decimal3 );

    }
}
