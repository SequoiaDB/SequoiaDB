package com.sequoiadb.crud.numoverflow;

import java.math.BigDecimal;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * FileName: AddIsMatcher12593.java test content:Numeric value overflow for
 * single character using $add operation, and the $add is used as a matcher.
 * testlink case:seqDB-12593
 * 
 * @author luweikang
 * @Date 2017.9.13
 * @version 1.00
 */

public class AddIsMatcher12593 extends SdbTestBase {

    private String clName = "add_matcher12593";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = {
                "{'no1':-2147483648,'long':{'$numberLong':'-9223372036854775808'}}" };
        String[] expRecords2 = { "{'no2':{a:{b:-2147483648}},'string':'123'}" };
        String[] expRecords3 = {
                "{'no3':[2147483647,{'$numberLong':'9223372036854775807'}]}" };

        return new Object[][] {
                // the parameters: matcherName,matcherValue,expRecords
                new Object[] { "no1", new Integer( -2147483648 ),
                        new Long( -4294967296L ), expRecords1 },
                new Object[] { "long", new Long( -1 ),
                        new BigDecimal( "-9223372036854775809" ), expRecords1 },
                new Object[] { "no2.a.b", new Integer( -2147483648 ),
                        new Long( -4294967296L ), expRecords2 },
                new Object[] { "string", new Integer( 321 ), null,
                        expRecords2 },
                new Object[] { "no3.0", new Integer( 2147483647 ),
                        new Long( 4294967294L ), expRecords3 },
                new Object[] { "no3.1", new Long( 10L ),
                        new BigDecimal( "9223372036854775817" ), expRecords3 },

        };
    }

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
                "{'no1':-2147483648,'long':{'$numberLong':'-9223372036854775808'}}",
                "{'no2':{a:{b:-2147483648}},'string':'123'}",
                "{'no3':[2147483647,{'$numberLong':'9223372036854775807'}]}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testAdd( String matcherName, Object matcherValue,
            Object resultValue, String[] expRecords ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            mValue.put( "$add", matcherValue );
            mValue.put( "$et", resultValue );
            NumOverflowUtils.matcherOper( cl, matcherName, mValue, expRecords );

        } catch ( BaseException e ) {
            Assert.assertTrue( false, "add data is used as matcher oper failed,"
                    + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
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
