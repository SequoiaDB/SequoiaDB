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
 * FileName: MultiplyIsMatcher12595.java test content:Numeric value overflow for
 * single character using $multiply operation, and the $multiply is used as a
 * matcherSymbol. testlink case:seqDB-12595
 * 
 * @author wuyan
 * @Date 2017.9.13
 * @version 1.00
 */

public class MultiplyIsMatcher12595 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'a':-2147483648,'b':{'$numberLong':'-1024819115206086201'}}" };
        String[] expRecords2 = {
                "{a:[{'$numberLong':'-1024819115206086201'},3,-715827883],"
                        + "b:[{a:{'$numberLong':'36854775808'}},'testo1']}" };

        return new Object[][] {
                // parameters:
                // matcherName,arithmeticValue,resultVaule,expRecords
                // int32 * int32 type numberflow:2147483648L = (-1)*a
                new Object[] { "a", new Integer( -1 ), new Long( 2147483648L ),
                        expRecords1 },
                // int32(arr) * int32 type
                // numberflow:2147483649L=715827883*(a.1)
                new Object[] { "a.1", new Integer( 715827883 ),
                        new Long( 2147483649L ), expRecords2 },
                // int64 * int64 type
                // numberflow:Decimal(-9223372036854775809)=9L*(b.0)
                new Object[] { "b", new Long( 9 ),
                        new BSONDecimal( "-9223372036854775809" ),
                        expRecords1 },
                // int64 * int64 type
                // numberflow:Decimal(9223372036854775809)=9L*(b.0)
                new Object[] { "b.0.a", new Long( 250262611 ),
                        new BSONDecimal( "9223372421529714688" ),
                        expRecords2 }, };
    }

    private String clName = "multiply_matcher12595";
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

        String clOption = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{'a':-2147483648,'b':{'$numberLong':'-1024819115206086201'}}",
                "{a:[{'$numberLong':'-1024819115206086201'},3,-715827883],"
                        + "b:[{a:{'$numberLong':'36854775808'}},'testo1']}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testMultiplyAsMatcher( String matcherName,
            Object arithmeticValue, Object resultVaule, String[] expRecords ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            mValue.put( "$multiply", arithmeticValue );
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
