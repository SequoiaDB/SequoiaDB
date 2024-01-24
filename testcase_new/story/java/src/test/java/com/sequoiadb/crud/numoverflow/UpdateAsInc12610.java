package com.sequoiadb.crud.numoverflow;

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
import com.sequoiadb.crud.numoverflow.NumOverflowUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: UpdateAsInc12610.java test content:Numeric value overflow for
 * single character using $inc operation, and $inc data is same data Type.
 * testlink case:seqDB-12610
 * 
 * @author wuyan
 * @Date 2017.9.14
 * @version 1.00
 */

public class UpdateAsInc12610 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{no:2147483649,a:{'$numberLong':'-9223372036854775808'},test:0}" };
        String[] expRecords2 = {
                "{no:[-2147483649,{'$numberLong':'-8223372036854775800'}],a:[1,[2147483648],3],test:1}" };
        String[] expRecords3 = {
                "{no:[-2147483649,{'$numberLong':'-8223372036854775800'}],a:[1,[2147483649],3],test:1}" };
        String[] expRecords01 = {
                "{no:2147483649,a:{'$decimal':'-9223372036854775809'},test:0}" };
        String[] expRecords02 = {
                "{no:[-2147483649,{'$decimal':'-9223372036854775809'}],a:[1,[2147483649],3],test:1}" };
        String[] expRecords03 = {
                "{no:2147,a:[1,{a:{b:{c:{'$decimal':'9223372036854775808'}}}},2],test:2}" };

        String expDecimalType = "decimal";
        String expLongType = "int64";
        String expLongTypeToJava = "class java.lang.Long";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";
        return new Object[][] {
                // the parameters:
                // matcherValue,updateName,incValue,expRecords,expType,isVerifyTypeTojava,expTypeToJava
                // ---int32 and int32
                // -2147483648 inc 1,the result is -2147483649(int64)
                new Object[] { 0, "no", 2147483001, expRecords1, expLongType,
                        true, expLongTypeToJava },
                // array: no.0 inc -1,the result is -2147483649(int64)
                new Object[] { 1, "no.0", -1, expRecords2, expLongType, false,
                        null },
                // array: no.1.0 inc 1,the result is 2147483649(int64)
                new Object[] { 1, "a.1.0", 1, expRecords3, expLongType, false,
                        null },

                // ---int64 and int64
                // -9223372036854775808 inc -1,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 0, "a", new Long( -1 ), expRecords01,
                        expDecimalType, true, expDecimalTypeToJava },
                // arr.1 inc -1,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 1, "no.1", new Long( -1000000000000000009L ),
                        expRecords02, expDecimalType, false, null },
                // a.1.a.b.c inc 1,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 2, "a.1.a.b.c", new Long( 1 ), expRecords03,
                        expDecimalType, false, null }, };
    }

    private String clName = "inc_update12610";
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

        String clOption = "{ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{no:648,a:{'$numberLong':'-9223372036854775808'},test:0}",
                "{no:[-2147483648,{'$numberLong':'-8223372036854775800'}],a:[1,[2147483648],3],test:1}",
                "{no:2147,a:[1,{a:{b:{c:{'$numberLong':'9223372036854775807'}}}},2],test:2}" };
        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testUpdate( int matcherValue, String updateName,
            Object incValue, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) {
        try {
            BSONObject updateValue = new BasicBSONObject();
            updateValue.put( updateName, incValue );
            NumOverflowUtils.updateOper( cl, matcherValue, updateValue,
                    "update" );
            NumOverflowUtils.checkUpdateResult( cl, matcherValue, expRecords );
            // TODO:SEQUOIADBMAINSTREAM-2795
            if ( !updateName.contains( "." ) ) {
                try {
                    NumOverflowUtils.checkUpdateDataType( cl, matcherValue,
                            updateName, expTypeToSdb, isVerifyTypeToJava,
                            typeToJava );
                } catch ( Exception e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "update numeric value overflowoper failed," + e.getMessage()
                            + e.getErrorCode() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.getCollectionSpace( SdbTestBase.csName )
                    .isCollectionExist( clName ) ) {
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
