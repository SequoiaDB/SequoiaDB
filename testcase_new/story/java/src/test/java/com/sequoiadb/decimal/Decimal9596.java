package com.sequoiadb.decimal;

import java.math.BigDecimal;
import java.util.Date;
import java.util.Map;

import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Decimal9596.java
 * TestLink: seqDB-9596
 * 
 * @author zhaoyu
 * @Date 2016.9.29
 * @version 1.00
 */

public class Decimal9596 {

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
        BasicBSONList list = new BasicBSONList();
        BSONDecimal decimal1 = null;
        BSONDecimal decimal2 = null;
        BSONDecimal decimal3 = null;
        BSONDecimal actualDecimal1 = null;
        BSONDecimal actualDecimal2 = null;
        BSONDecimal actualDecimal3 = null;
        try {
            String value1 = "1123456789.12345678901234";
            BigDecimal bigDecimal = new BigDecimal( value1 );
            decimal1 = new BSONDecimal( bigDecimal );

            String value2 = "170E308";
            decimal2 = new BSONDecimal( value2 );

            String value3 = "-49406.5645841246544e-350";
            int precision = 1000;
            int scale = 500;
            decimal3 = new BSONDecimal( value3, precision, scale );

            list.put( "1", decimal1 );
            list.put( "3", decimal2 );
            list.put( "5", decimal3 );

            @SuppressWarnings("unchecked")
            Map< String, Object > map = list.toMap();
            actualDecimal1 = ( BSONDecimal ) map.get( "1" );
            actualDecimal2 = ( BSONDecimal ) map.get( "3" );
            actualDecimal3 = ( BSONDecimal ) map.get( "5" );
        } catch ( IllegalArgumentException e ) {
            Assert.fail( "generate data failed, errMsg:" + e.getMessage() );
        }

        Assert.assertEquals( actualDecimal1, decimal1 );
        Assert.assertEquals( actualDecimal2, decimal2 );
        Assert.assertEquals( actualDecimal3, decimal3 );

    }
}
