package com.sequoiadb.decimal;

import java.math.BigDecimal;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Decimal9581.java
 * TestLink: seqDB-9581/seqDB-9582/seqDB-9583/seqDB-9584/seqDB-9588/seqDB-9589
 * 
 * @author zhaoyu
 * @Date 2016.9.26
 * @version 1.00
 */
public class Decimal9581 extends SdbTestBase {
    private Sequoiadb sdb;

    private CollectionSpace cs = null;
    private String clName = "cl9581";
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
                // seqDB-9581
                { "-92233720368547758081", "-92233720368547758081", 20, 0 },
                { "92233720368547758082", "92233720368547758082", 20, 0 },
                { "0", "0.000", 5, 3 },
                { "112233.112233445566778899", "112233.112233445566778899", 24,
                        18 },
                { "-123.112233445566778899", "-123.112233445566778899", 21,
                        18 },
                { ".112233445566778899", "0.112233445566778899", 19, 18 },
                { "-.2", "-0.2", 2, 1 },
                { "1.7e+310",
                        "17000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.00",
                        1000, 2 },
                { "-1.7E310",
                        "-17000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                        1000, 0 },
                { "-4.94065645841246544E-329",
                        "-0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004940656458412465440000",
                        1000, 350 },
                { "4.94065645841246544e-329",
                        "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004940656458412465440000",
                        1000, 350 },
                // seqDB-9582
                { "1", "1", 1, 0 }, { "1.2", "1.20", 500, 2 },
                { "1", "1.00", 1000, 2 },
                // seqDB-9583
                { "1", "1", 1000, 0 },
                { "1", "1.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                        1000, 499 },
                { "1", "1.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                        1000, 999 },
                // seqDB-9584
                { "123456", "123456.0", 7, 1 }, { "1.23456", "1.2346", 6, 4 },
                { "1.23456", "1.2346", 10, 4 }, { "1.23456", "1.23", 3, 2 },
                { "1.23456", "1.23", 10, 2 }, { "1.23456", "1", 1, 0 },
                { "1.53456", "2", 1, 0 }, };
    }

    @Test(dataProvider = "createDataProvider")
    public void test( String value, String expectValue, int precision,
            int scale ) {
        // insert data and check
        insertAndCheckDecimal( value, expectValue, precision, scale );
    }

    @DataProvider(name = "illegalDataProvider")
    public Object[][] createillegalData() {
        return new Object[][] {
                // seqDB-9581
                { "a", 6, 5 },
                // seqDB-9582
                { "1", 0, 0 }, { "1", 1001, 2 }, { "1", -1, 0 },
                // seqDB-9583
                { "1", 1000, 1000 }, { "1", 1000, -1 },
                // seqDB-9584
                { "123.2", 5, 5 }, { "1", 5, 7 }, { "123", 6, 4 },
                { "123.456", 6, 4 }, };
    }

    @Test(dataProvider = "illegalDataProvider")
    public void testAbnormal( String value, int precision, int scale ) {
        // illegaData check
        try {
            BSONDecimal illegalData = new BSONDecimal( value, precision,
                    scale );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( IllegalArgumentException e ) {
        }
    }

    public void insertAndCheckDecimal( String value, String expectValue,
            int precision, int scale ) {
        BSONObject obj = new BasicBSONObject();
        try {
            BigDecimal expectBigDecimal = new BigDecimal( expectValue );
            BSONDecimal data = new BSONDecimal( value, precision, scale );
            obj.put( "a", data );
            cl.insert( obj );
            BSONDecimal actualData = ( BSONDecimal ) cl.queryOne().get( "a" );
            BigDecimal actualBigDecimal = actualData.toBigDecimal();
            String actualValue = data.getValue();
            int actualPrecision = data.getPrecision();
            int actualScale = data.getScale();
            Assert.assertEquals( actualData, data );
            Assert.assertEquals( actualValue, expectValue );
            Assert.assertEquals( actualPrecision, precision );
            Assert.assertEquals( actualScale, scale );
            if ( actualBigDecimal.compareTo( expectBigDecimal ) != 0 ) {
                Assert.fail( "compare bigDecimal data failed,expect data:"
                        + expectBigDecimal + ",actual data: "
                        + actualBigDecimal );
            }
            cl.truncate();
        } catch ( IllegalArgumentException e ) {
            Assert.fail( "generate data:" + obj + " failed, errMsg:"
                    + e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( "insert data:" + obj + " failed, errMsg:"
                    + e.getMessage() );
        }
    }

}
