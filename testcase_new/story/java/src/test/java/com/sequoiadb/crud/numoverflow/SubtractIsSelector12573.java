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
 * FileName: SubtractIsSelector12573.java test content:Numeric value overflow
 * for single character using $subtract operation, and the $subtract is used as
 * a selector. testlink case:seqDB-12573
 * 
 * @author wuyan
 * @Date 2017.9.4
 * @version 1.00
 */

public class SubtractIsSelector12573 extends SdbTestBase {

    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'no':-2147483649,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        String[] expRecords2 = {
                "{no:2147483648,'long':{'$numberLong':'9223372036854775807'},test:1}" };
        String[] expRecords3 = {
                "{no:2194966294,'long':{'$numberLong':'-8223372036854775807'},arr:[1000000000,-2100],test:2}" };
        String[] expRecords4 = {
                "{no:1147483147,long:-8223372036854775807,arr:[ -1147481549,-2147483649],test:2}}" };
        String expLongType = "int64";
        String expLongTypeToJava = "class java.lang.Long";
        String[] expRecords5 = {
                "{no:1147483147,long:-8223372036854775807,arr:[ -2147483649],test:2}}" };

        String[] expRecords01 = {
                "{'no':-2147483648,'tlong':{'$decimal':'-9223372036854775809'},'test':0}" };
        String[] expRecords02 = {
                "{'no':2147483647,'long':{'$decimal':'9223372036854775808'},'test':1}" };
        String[] expRecords03 = {
                "{no:1147483147,'long':{'$decimal':'-9223372036854775809'},"
                        + "arr:[1000000000,-2100],test:2}" };
        String[] expRecords04 = {
                "{no:1147483147,'long':{'$numberLong':'-8223372036854775807'},"
                        + "arr:[{'$decimal':'9223372036854775808'},9223372035854773708],test:2}" };
        String[] expRecords05 = {
                "{no:[{'$decimal':'9223372036854775809'}],arr:[1000000000,-2147483648],test:3}" };
        String expDecimalType = "decimal";
        String expArrayType = "array";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";

        return new Object[][] {
                // parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyTypeToJava,
                // expTypeTojava
                // ---int32 and int32
                // -2147483648 $substract 1,the result is -2147483649(int64)
                new Object[] { 0, 1, "no", expRecords1, expLongType, true,
                        expLongTypeToJava },
                // 2147483647 $substract -1,the result is 2147483648(int64)
                new Object[] { 1, -1, "no", expRecords2, expLongType, true,
                        expLongTypeToJava },
                // 1147483147 $substract -1047483147,the result is
                // 2194966294(int64)
                new Object[] { 2, -1047483147, "no", expRecords3, expLongType,
                        true, expLongTypeToJava },
                //// the arr -2100 $substract 2147481549,the result is
                //// -2147483649(int64)
                new Object[] { 2, 2147481549, "arr", expRecords4, expArrayType,
                        false, null },
                // the arr.1 -2100 $substract 2147481549,the result is
                // -2147483649(int64)
                new Object[] { 2, 2147481549, "arr.$[1]", expRecords5, null,
                        false, null },

                // ---int64 and int64
                // tlong:-9223372036854775808 $substract 1,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 0, new Long( 1 ), "tlong", expRecords01,
                        expDecimalType, true, expDecimalTypeToJava },
                // long:9223372036854775807 $substract -1,the result is
                // {'$decimal':'9223372036854775809'}
                new Object[] { 1, new Long( -1 ), "long", expRecords02,
                        expDecimalType, true, expDecimalTypeToJava },
                // long:-8223372036854775807 $substract 1000000000000000002,the
                // result is {'$decimal':'-9223372036854775809'}
                new Object[] { 2, new Long( 1000000000000000002L ), "long",
                        expRecords03, expDecimalType, true,
                        expDecimalTypeToJava },
                // arr:[1000000000,-2147483648] $substract -8223372036854775809
                new Object[] { 2, new Long( -9223372035854775808L ), "arr",
                        expRecords04, expArrayType, false, null },
                // no.$[1]:8223372036854775807 $substract
                // -1000000000000000002,the result is
                // {'$decimal':'9223372036854775809'}
                new Object[] { 3, new Long( -1000000000000000002L ), "no.$[1]",
                        expRecords05, expDecimalType, false, null }, };
    }

    private String clName = "subtract_selector12573";
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
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}",
                "{no:2147483647,'long':{'$numberLong':'9223372036854775807'},test:1}",
                "{no:1147483147,'long':{'$numberLong':'-8223372036854775807'},arr:[1000000000,-2100],test:2}",
                "{no:[2147483147,{'$numberLong':'8223372036854775807'}],arr:[1000000000,-2147483648],test:3}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSubtract( int matcherValue, Object subValue,
            String selectorName, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) {
        try {

            BSONObject sValue = new BasicBSONObject();
            sValue.put( "$subtract", subValue );
            NumOverflowUtils.selectorOper( cl, matcherValue, sValue,
                    selectorName, expRecords );
            try {
                NumOverflowUtils.checkDataType( cl, sValue, matcherValue,
                        selectorName, expTypeToSdb, isVerifyTypeToJava,
                        typeToJava );
            } catch ( Exception e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "subtract intData is used as selector oper failed,"
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
