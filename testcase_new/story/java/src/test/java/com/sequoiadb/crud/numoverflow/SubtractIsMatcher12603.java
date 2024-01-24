package com.sequoiadb.crud.numoverflow;

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
import com.sequoiadb.crud.numoverflow.NumOverflowUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: SubtractIsMatcher12603.java test content:Numeric value overflow for
 * single character using $subtract operation, and the $subtract is used as a
 * matcherSymbol. testlink case:seqDB-12603
 * 
 * @author wuyan
 * @Date 2017.9.14
 * @version 1.00
 */

public class SubtractIsMatcher12603 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'a':-2147483648,'b':{'$numberLong':'-9223372036854770000'}}" };
        String[] expRecords2 = {
                "{a:[1,[3,214748,[-1,2147483647]]],b:{a:{c:{int:12344,long:{'$numberLong':'9223372034707292160'}}}}}" };
        String[] expRecords3 = {
                "{a:[{'$numberLong':'-9223372036854775808'},3],b:{a:{'$numberLong':'9223372036854775807'}}}" };
        return new Object[][] {
                // parameters:
                // matcherName,arithmeticValue,resultVaule,expRecords
                // int32 - int64 type numberflow:a-1=-2147483649L
                new Object[] { "a", new Integer( 1 ), new Long( -2147483649L ),
                        expRecords1 },
                // int32(arr) - int64 type numberflow:a.1.2.1 -
                // 9223372034707292161L = Decimal(9223372036854775808)
                new Object[] { "a.1.2.1", new Long( -9223372034707292161L ),
                        new BSONDecimal( "9223372036854775808" ), expRecords2 },
                // int64 - int32 type numberflow: b -
                // 5800=Decimal(-9223372036854775809)
                new Object[] { "b", new Integer( 5809 ),
                        new BSONDecimal( "-9223372036854775809" ),
                        expRecords1 },
                // int64(arr) - int32 numberflow:a.0 -1 =
                // Decimal(-9223372036854775809)
                new Object[] { "a.0", new Integer( 1 ),
                        new BSONDecimal( "-9223372036854775809" ),
                        expRecords3 },
                // int64(obj) - int32 numberflow:b.a - (-1)=
                // Decimal(9223372036854775808)
                new Object[] { "b.a", new Integer( -1 ),
                        new BSONDecimal( "9223372036854775808" ), expRecords3 },
                // int64(obj:b.a.c.long) - int32 numberflow:b.a.c.long - (-1)
                new Object[] { "b.a.c.long", new Integer( -2147483648 ),
                        new BSONDecimal( "9223372036854775808" ), expRecords2 },

        };
    }

    private String clName = "subtract_matcher12603";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName );

        String[] records = {
                "{'a':-2147483648,'b':{'$numberLong':'-9223372036854770000'}}",
                "{a:[{'$numberLong':'-9223372036854775808'},3],b:{a:{'$numberLong':'9223372036854775807'}}}",
                "{a:[1,[3,214748,[-1,2147483647]]],b:{a:{c:{int:12344,long:{'$numberLong':'9223372034707292160'}}}}}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testAddAsMatcher( String matcherName, Object arithmeticValue,
            Object resultVaule, String[] expRecords ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            mValue.put( "$subtract", arithmeticValue );
            mValue.put( "$et", resultVaule );

            NumOverflowUtils.matcherOper( cl, matcherName, mValue, expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "abs intData is used as selector oper failed,"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {

            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

}
