package com.sequoiadb.decimal;

import java.math.BigDecimal;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Decimal9593.java
 * TestLink: seqDB-9593
 * 
 * @author zhaoyu
 * @Date 2016.9.29
 * @version 1.00
 */
public class Decimal9593 extends SdbTestBase {
    private Sequoiadb sdb;

    private CollectionSpace cs = null;
    private String clName = "cl9593";
    private DBCollection cl = null;
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
                sdb.createCollectionSpace( commCSName );
            }
            cs = sdb.getCollectionSpace( commCSName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @DataProvider(name = "createDataProvider")
    public Object[][] createData() {
        return new Object[][] {
                { "123456789.123456789", "123456789.1234567890", 100, 10 },
                { "-123456789.123456789", "-123456789.1234567890", 100, 10 },
                { "11223344556677889900", "11223344556677889900", 100, 0 },
                { "-11223344556677889900", "-11223344556677889900", 100, 0 },

        };
    }

    @Test(dataProvider = "createDataProvider")
    public void test( String value, String expectValue, int precision,
            int scale ) {

        DecimalBean decimalBean = new DecimalBean();
        try {
            BigDecimal bigDecimal = new BigDecimal( value );
            decimalBean.setBigDecimal( bigDecimal );

            BSONDecimal decimalNoPrecision = new BSONDecimal( value );
            decimalBean.setDecimalNoPrecision( decimalNoPrecision );

            BSONDecimal decimalWithPrecision = new BSONDecimal( value,
                    precision, scale );
            decimalBean.setDecimalWithPrecision( decimalWithPrecision );

            cl.save( decimalBean );
        } catch ( BaseException e ) {
            Assert.fail( "insert data:" + decimalBean + " failed, errMsg:"
                    + e.getMessage() );
        }

        DecimalBean actualDecimalBean = new DecimalBean();
        BSONObject retObj = null;
        try {
            retObj = cl.queryOne();
            actualDecimalBean = retObj.as( DecimalBean.class );
        } catch ( Exception e ) {
            Assert.fail( "BSONObject as DecimalBean failed, errMsg:"
                    + e.getMessage() );
        }

        int actualPrecision1 = 0;
        int actualScale1 = 0;
        String actualValue1 = null;

        int actualPrecision2 = 0;
        int actualScale2 = 0;
        String actualValue2 = null;

        int actualPrecision3 = 0;
        int actualScale3 = 0;
        String actualValue3 = null;
        try {
            BSONDecimal d1 = ( BSONDecimal ) retObj.get( "bigDecimal" );
            actualPrecision1 = d1.getPrecision();
            actualScale1 = d1.getScale();
            actualValue1 = d1.getValue();

            BSONDecimal d2 = ( BSONDecimal ) retObj.get( "decimalNoPrecision" );
            actualPrecision2 = d2.getPrecision();
            actualScale2 = d2.getScale();
            actualValue2 = d2.getValue();

            BSONDecimal d3 = ( BSONDecimal ) retObj
                    .get( "decimalWithPrecision" );
            actualPrecision3 = d3.getPrecision();
            actualScale3 = d3.getScale();
            actualValue3 = d3.getValue();

            cl.truncate();
        } catch ( IllegalArgumentException e ) {
            Assert.fail( "get decimal data failed, errMsg:" + e.getMessage() );
        }

        // case 1
        // check retObj
        Assert.assertEquals( actualPrecision1, -1 );
        Assert.assertEquals( actualScale1, -1 );
        Assert.assertEquals( actualValue1, value );

        Assert.assertEquals( actualPrecision2, -1 );
        Assert.assertEquals( actualScale2, -1 );
        Assert.assertEquals( actualValue2, value );

        Assert.assertEquals( actualPrecision3, precision );
        Assert.assertEquals( actualScale3, scale );
        Assert.assertEquals( actualValue3, expectValue );

        // case 2
        Assert.assertEquals( actualDecimalBean, decimalBean );

    }
}
