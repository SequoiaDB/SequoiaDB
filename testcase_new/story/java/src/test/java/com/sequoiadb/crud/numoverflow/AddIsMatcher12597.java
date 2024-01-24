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
 * FileName: AddIsMatcher12597.java test content:different types of numerical
 * operations, the result of the operation is converted to a high-level type
 * testlink case:seqDB-12597
 * 
 * @author wuyan
 * @Date 2017.9.14
 * @version 1.00
 */

public class AddIsMatcher12597 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'a':-2147483648,'b':{'$numberLong':'-1024819115206086201'}}" };
        String[] expRecords2 = { "{'a':[1,[3,214748,[-1,2147483647]]],'b':1}" };
        String[] expRecords3 = {
                "{a:[{'$numberLong':'-9223372036854775808'},3],"
                        + "b:[{a:{'$numberLong':'9223372036854775807'}},'testo1']}" };
        return new Object[][] {
                // parameters:
                // matcherName,arithmeticValue,resultVaule,expRecords
                // int32 + doulbe type --result is int64
                new Object[] { "a", new Double( -1.0 ),
                        new Double( -2147483649.0 ), expRecords1 },
                // int32(arr) + int64 --result is int64
                new Object[] { "a.1.2.1", new Long( 9223372034707292160L ),
                        new Long( "9223372036854775807" ), expRecords2 },
                // int64 + double --result is double
                new Object[] { "b", new Double( -8198552921648689608.01 ),
                        new Double( "-9223372036854775809.01" ), expRecords1 },
                // int64(arr) + decimal --result is decimal
                new Object[] { "a.0", new BSONDecimal( "1234" ),
                        new BSONDecimal( "-9223372036854774574" ),
                        expRecords3 },
                // int64(obj) + int32 --result is int64
                new Object[] { "b.0.a", new Integer( -1 ),
                        new Long( "9223372036854775806" ), expRecords3 }, };
    }

    private String clName = "add_matcher12597";
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
                "{'a':-2147483648,'b':{'$numberLong':'-1024819115206086201'}}",
                "{a:[{'$numberLong':'-9223372036854775808'},3],b:[{a:{'$numberLong':'9223372036854775807'}},'testo1']}",
                "{a:[1,[3,214748,[-1,2147483647]]],b:1}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testAddAsMatcher( String matcherName, Object arithmeticValue,
            Object resultVaule, String[] expRecords ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            mValue.put( "$add", arithmeticValue );
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
