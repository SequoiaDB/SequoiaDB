package com.sequoiadb.basicoperation;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.SdbTestBase;

public class TestCompareNumberLong13314 extends SdbTestBase {

    @BeforeClass
    public void setUp() {
    }

    @Test
    public void testCompare() {

        // 比较不相等的数值
        BSONObject exp1 = new BasicBSONObject( "no",
                new Long( 8223372036854775296L ) );
        BSONObject exp2 = new BasicBSONObject( "no",
                new Long( 8223372036854775807L ) );
        Assert.assertNotEquals( exp1, exp2,
                "numberLong compare result was wrong" );

        // 比较相等的数值
        BSONObject exp3 = new BasicBSONObject( "no",
                new Long( 8223372036854775800L ) );
        BSONObject exp4 = new BasicBSONObject( "no",
                new Long( 8223372036854775800L ) );
        Assert.assertEquals( exp3, exp4,
                "numberLong compare result was wrong" );

        // 比较不同类型相等的数值
        BSONObject exp5 = new BasicBSONObject( "no", new Long( 1L ) );
        BSONObject exp6 = new BasicBSONObject( "no", new Integer( 1 ) );
        Assert.assertNotEquals( exp5, exp6,
                "numberLong compare result was wrong" );

    }

    @AfterClass
    public void tearDown() {
    }
}
