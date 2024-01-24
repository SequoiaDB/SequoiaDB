package com.sequoiadb.crud.numoverflow;

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
 * FileName: UpdateAsInc12613.java test content:Numeric value overflow for
 * shardingKey using $inc operation, testlink case:seqDB-12613
 * 
 * @author wuyan
 * @Date 2017.9.14
 * @version 1.00
 */

public class UpdateAsInc12613 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{no:2147483649,a:{'$numberLong':'-9223372036854775808'},test:0}" };
        String[] expRecords2 = {
                "{no:[-2147483648,{'$decimal':'-9223372036854775809'}],a:1,test:1}" };
        String[] expRecords3 = {
                "{no:2147483649,a:{'$decimal':'-9223372036854775809'},test:0}" };
        String[] expRecords4 = {
                "{no:2147,a:{b:{c:{'$decimal':'9223372036854775808'}}},test:2}" };

        String expDecimalType = "decimal";
        String expLongType = "int64";
        String expLongTypeToJava = "class java.lang.Long";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";
        return new Object[][] {
                // the parameters:
                // matcherValue,updateName,incValue,expRecords,expType,isVerifyTypeTojava,expTypeToJava
                //
                new Object[] { 0, "no", new Integer( 2147483001 ), expRecords1,
                        expLongType, true, expLongTypeToJava },
                // int64(arr.1)/int64:the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 1, "no.1", new Long( -1000000000000000009L ),
                        expRecords2, expDecimalType, false, null },
                // int64/int32:the result is {'$decimal':'-9223372036854775809'}
                new Object[] { 0, "a", new Integer( -1 ), expRecords3,
                        expDecimalType, true, expDecimalTypeToJava },
                // a.b.c inc 1,the result is {'$decimal':'9223372036854775808'}
                new Object[] { 2, "a.b.c", new Long( 1 ), expRecords4,
                        expDecimalType, false, null }, };
    }

    private String clName = "inc_update12613";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    // SEQUOIADBMAINSTREAM-3206
    @BeforeClass(enabled = false)
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        String clOption = "{ShardingKey:{no:1,a:-1},ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{no:648,a:{'$numberLong':'-9223372036854775808'},test:0}",
                "{no:[-2147483648,{'$numberLong':'-8223372036854775800'}],a:1,test:1}",
                "{no:2147,a:{b:{c:{'$numberLong':'9223372036854775807'}}},test:2}" };
        NumOverflowUtils.insert( cl, records );
    }

    // SEQUOIADBMAINSTREAM-3206
    @Test(enabled = false, dataProvider = "operData")
    public void testUpdateShardingKey( int matcherValue, String updateName,
            Object incValue, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) {
        try {
            BSONObject updateValue = new BasicBSONObject();
            updateValue.put( updateName, incValue );
            NumOverflowUtils.updateOper( cl, matcherValue, updateValue,
                    "updateShardingKey" );
            NumOverflowUtils.checkUpdateResult( cl, matcherValue, expRecords );
            // TODO:SEQUOIADBMAINSTREAM-2795
            if ( !updateName.contains( "." ) ) {
                try {
                    NumOverflowUtils.checkUpdateDataType( cl, matcherValue,
                            updateName, expTypeToSdb, isVerifyTypeToJava,
                            typeToJava );
                } catch ( Exception e ) {
                    e.printStackTrace();
                }
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "update numeric value overflowoper failed," + e.getMessage()
                            + e.getErrorCode() );
        }
    }

    // SEQUOIADBMAINSTREAM-3206
    @AfterClass(enabled = false)
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
