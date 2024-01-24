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
 * FileName: DivideIsSelector12576.java test content:Numeric value overflow for
 * single character using $divide operation, and the $divide is used as a
 * selector. testlink case:seqDB-12576
 * 
 * @author wuyan
 * @Date 2017.9.11
 * @version 1.00
 */

public class DivideIsSelector12576 extends SdbTestBase {

    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'no':{'$numberLong':'2147483648'},'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        String[] expRecords2 = {
                "{no: [ 2147483648] ,'obj':{'a':-2147483648},'test':1}" };
        // String []expRecords3 =
        // {"{no:[1,[{'$numberLong':'2147483648'}],3],obj:{a:[1,{'$numberLong':'-9223372036854775808'}]},test:3}"};
        String[] expRecords4 = {
                "{no:[-2147483648,{'$numberLong':'-9223372036854775808'}],obj:{a:{'$numberLong':'2147483648'}},test:1}" };
        String[] expRecords5 = {
                "{no:{'$numberLong':'33'},obj:{a:{b:{int:{'$numberLong':'2147483648'},long:{'$numberLong':'-9223372036854775808'}}}},test:2}" };
        String[] expRecords6 = {
                "{no:{'$numberLong':'8'},obj:{a:{b:{int:-2147483648,long:{'$numberLong':'-9223372036854775808'}}}},test:2}" };

        String[] expRecords01 = {
                "{'no':-2147483648,'tlong':{'$decimal':'9223372036854775808'},'test':0}" };
        String[] expRecords02 = {
                "{'no':[{'$decimal':'9223372036854775808'}],'obj':{a:-2147483648},'test':1}" };
        String[] expRecords03 = {
                "{no:{'$numberLong':'33'},obj:{a:{b:{int:-2147483648,"
                        + "long:{'$decimal':'9223372036854775808'}}}},test:2}" };
        String[] expRecords04 = {
                "{no:[1,[-2147483648],3],obj:{a:[{'$decimal':'9223372036854775808'}]},test:3}" };
        String expDecimalType = "decimal";
        String expLongType = "int64";
        String expLongTypeToJava = "class java.lang.Long";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";
        return new Object[][] {
                // the parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyTypeTojava,expTypeToJava
                // ---int32 and int32
                // -2147483648 / -1,the result is 2147483648(int64)
                new Object[] { 0, -1, "no", expRecords1, expLongType, true,
                        expLongTypeToJava },
                // array: no.$[0] / -1,the result is 2147483648(int64)
                new Object[] { 1, -1, "no.$[0]", expRecords2, expLongType,
                        false, null },
                // array: no.$[1].$[0] / -1,the result is 2147483648(int64)
                // new Object[]{3, -1,
                // "no.$[1].$[0]",expRecords3,expLongType,true},
                // the obj obj.a / -1,the result is 2147483648(int64)
                new Object[] { 1, -1, "obj.a", expRecords4, expLongType, false,
                        null },
                // the obj obj.a.b.int /-1 ,the result is 2147483648(int64)
                new Object[] { 2, -1, "obj.a.b.int", expRecords5, expLongType,
                        false, null },
                // not divisible:33/4=8
                new Object[] { 2, 4, "no", expRecords6, expLongType, false,
                        null },

                // ---int64 and int64
                // -9223372036854775808 / -1,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 0, new Long( -1 ), "tlong", expRecords01,
                        expDecimalType, true, expDecimalTypeToJava },
                // arr.1 / -1,the result is {'$decimal':'9223372036854775808'}
                new Object[] { 1, new Long( -1 ), "no.$[1]", expRecords02,
                        expDecimalType, false, null },
                // the obj obj.a.b.long /-1,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 2, new Long( -1 ), "obj.a.b.long", expRecords03,
                        expDecimalType, false, null },
                // the arr.1 of obj:obj.a.$[1] /-1 ,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 3, new Long( -1 ), "obj.a.$[1]", expRecords04,
                        expDecimalType, false, null }, };
    }

    private String clName = "divide_selector12576";
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
                "{no:[-2147483648,{'$numberLong':'-9223372036854775808'}],obj:{a:-2147483648},test:1}",
                "{no:{'$numberLong':'33'},obj:{a:{b:{int:-2147483648,long:{'$numberLong':'-9223372036854775808'}}}},test:2}",
                "{no:[1,[-2147483648],3],obj:{a:[1,{'$numberLong':'-9223372036854775808'}]},test:3}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testDivide( int matcherValue, Object subValue,
            String selectorName, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) {
        try {

            BSONObject sValue = new BasicBSONObject();
            sValue.put( "$divide", subValue );
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
            Assert.assertTrue( false, "dividie is used as selector oper failed,"
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
