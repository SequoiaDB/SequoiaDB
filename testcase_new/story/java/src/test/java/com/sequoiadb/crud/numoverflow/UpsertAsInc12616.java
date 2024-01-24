package com.sequoiadb.crud.numoverflow;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
 * FileName: UpsertAsInc12616.java test content:Numeric value overflow for
 * update shardingKey using $inc operation,* testlink case:seqDB-12616
 * 
 * @author wuyan
 * @Date 2017.9.14
 * @version 1.00
 */

public class UpsertAsInc12616 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = { "{no:2147483648,test:0}" };
        String[] expRecords2 = {
                "{no:{a:{'$decimal':'-9223372036854775809'}},test:1}" };
        String[] expRecords3 = {
                "{no:{a:{b:{c:{'$decimal':'9223372036854775808'}}}},test:2}" };
        String expDecimalType = "decimal";
        String expLongType = "int64";
        String expLongTypeToJava = "class java.lang.Long";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";
        return new Object[][] {
                // the parameters:setInsertValue,
                // updateName,matcherValue,incValue,expRecords,expType,isVerifyTypeTojava,expTypeToJava
                // ---int32 and int64
                // -2147483648 inc 1,the result is -2147483649(int64)
                new Object[] { 0, "no", new Integer( 1 ),
                        new Integer( 2147483647 ), expRecords1, expLongType,
                        true, expLongTypeToJava },
                // obj: no.a inc -1,the result is -2147483649(int64)
                new Object[] { 1, "no.a", new Integer( -5 ),
                        new Long( -9223372036854775804L ), expRecords2,
                        expDecimalType, false, null },
                // -9223372036854775808 inc -1,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 2, "no.a.b.c", new Integer( 4 ),
                        new Long( 9223372036854775804L ), expRecords3,
                        expDecimalType, true, expDecimalTypeToJava },

        };
    }

    @DataProvider(name = "operData1")
    public Object[][] generateDatas1() {
        String[] expRecords = {
                "{no:[{'$decimal':'9223372036854775808'},3],test:10}" };
        String[] expRecords1 = {
                "{no:{a:[{'$decimal':'-9223372036854775809'},3]},test:11}" };
        String[] expRecords2 = { "{no:[-9,{a:2147462648}],test:12}" };
        String[] expRecords3 = {
                "{no:[-9,[3,{'$decimal':'9223372036854779000'}]],test:13}" };
        // String []expRecords4 =
        // {"{no:[23,{'$decimal':'-9223372036854775809'}],test:4}"};

        String matcher = "{no:[2147483647,3]}";
        String matcher1 = "{'no.a':[{'$numberLong':'-1000000000000000009'},3]}";
        String matcher2 = "{no:[-9,{a:2147462640}]}";
        String matcher3 = "{no:[-9,[3,4000]]}";
        // String matcher4 = "{no:[23,1]}";

        return new Object[][] {
                // the parameters:setInsertValue,
                // matcher,updateName,incValue,expRecords
                // array
                new Object[] { 10, matcher, "no.0",
                        new Long( 9223372034707292161L ), expRecords },
                // obj.arr
                new Object[] { 11, matcher1, "no.a.0",
                        new Long( -8223372036854775800L ), expRecords1 },
                // arr.obj
                new Object[] { 12, matcher2, "no.1.a", new Integer( 8 ),
                        expRecords2 },
                // arr.arr
                new Object[] { 13, matcher3, "no.1.1",
                        new Long( 9223372036854775000L ), expRecords3 }, };
    }

    private String clName = "inc_update12616";
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

        String clOption = "{ShardingKey:{no:1},ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{no:648,a:{'$numberLong':'-9223372036854775808'},test:8}" };
        NumOverflowUtils.insert( cl, records );
    }

    // SEQUOIADBMAINSTREAM-3206
    // @Test(dataProvider = "operData")
    public void testUpsert( int setInsertValue, String updateName,
            Object matcherValue, Object incValue, String[] expRecords,
            String expTypeToSdb, Boolean isVerifyTypeToJava,
            String typeToJava ) {
        try {
            BSONObject updateValue = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            updateValue.put( updateName, incValue );
            matcher.put( updateName, matcherValue );
            NumOverflowUtils.upsertOper( cl, matcher, updateValue,
                    setInsertValue, "upsertShardingKey" );
            NumOverflowUtils.checkUpdateResult( cl, setInsertValue,
                    expRecords );
            // TODO:SEQUOIADBMAINSTREAM-2795
            if ( !updateName.contains( "." ) ) {
                try {
                    NumOverflowUtils.checkUpdateDataType( cl, setInsertValue,
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
    @Test(enabled = false, dataProvider = "operData1")
    public void testUpsert1( int setInsertValue, String matcher,
            String updateName, Object incValue, String[] expRecords ) {
        try {
            BSONObject updateValue = new BasicBSONObject();
            BSONObject matcher0 = ( BSONObject ) JSON.parse( matcher );
            updateValue.put( updateName, incValue );
            NumOverflowUtils.upsertOper( cl, matcher0, updateValue,
                    setInsertValue, "upsertShardingKey" );
            NumOverflowUtils.checkUpdateResult( cl, setInsertValue,
                    expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "upsert numeric value overflowoper failed," + e.getMessage()
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
