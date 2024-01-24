package com.sequoiadb.crud.numoverflow;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-12585: 单个字段使用$subtract操作不同类型运算溢出
 * @author wuyan
 * @Date 2017.9.11
 * @updateUser wuyan
 * @updateDate 2021.9.3
 * @updateRemark 新增int32类型数值运算不溢出情况下保持原类型测试单
 * @version 1.00
 */

public class SubtractIsSelector12585 extends SdbTestBase {

    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'no':{'$decimal':'-9223372036854775809'},'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        // String []expRecords2 =
        // {"{arr:[1,0,[{'$decimal':'9223372036854775808'},1]],obj:{a:1},test:2}"};
        String[] expRecords3 = {
                "{arr:[1,0,[2147483147,1]],obj:{a:{'$decimal':'9223372036854775808'}},test:2}" };
        String[] expRecords01 = {
                "{'no':-2147483648,'tlong':{'$decimal':'-9223372036854775809'},'test':0}" };
        String[] expRecords02 = {
                "{arr:[{'$decimal':'9223372036854775808'}],obj:{a:{b:{int:2147483647,"
                        + "long:{'$numberLong':'9223372036854775800'}}}},test:1}" };
        String[] expRecords03 = {
                "{arr:[{'$numberLong':'9223372036854775807'},-2100],"
                        + "obj:{a:{b:{int:2147483647,long:{'$decimal':'9223372036854775808'}}}},test:1}" };
        String[] expRecords04 = {
                "{'no':-2147483640,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        String expDecimalType = "decimal";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";
        String expIntType = "int32";
        String expIntTypeToJava = "class java.lang.Integer";

        return new Object[][] {
                // parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyTypeToJava,
                // expTypeTojava
                // ---int32 and int64
                // no:-2147483648 $substract 9223372034707292161,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 0, new Long( 9223372034707292161L ), "no",
                        expRecords1, expDecimalType, true,
                        expDecimalTypeToJava },
                // arr.$[2].$[0]:1 $substract -9223372034707292661,the result is
                // {'$decimal':'9223372036854775808'}
                // TODO:SEQUOIADBMAINSTREAM-2764
                // new Object[]{2, new Long(-9223372034707292661L),
                // "arr.$[2].$[0]",expRecords2,expDecimalType,false,null},
                // obj.a:1 $substract -9223372036854775807,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 2, new Long( -9223372036854775807L ), "obj.a",
                        expRecords3, expDecimalType, false, null },

                // ---int64 and int32
                // tlong:-9223372036854775808 $substract 1,the result is
                // {'$decimal':'-9223372036854775809'}
                new Object[] { 0, new Integer( 1 ), "tlong", expRecords01,
                        expDecimalType, true, expDecimalTypeToJava },
                // arr.0:9223372036854775807 $substract -1,the result is
                // {'$decimal':'9223372036854775809'}
                new Object[] { 1, new Integer( -1 ), "arr.$[0]", expRecords02,
                        expDecimalType, false, null },
                // obj.a.b.long:9223372036854775800 $substract -8,the result is
                // {'$decimal':'9223372036854775808'}
                new Object[] { 1, new Integer( -8 ), "obj.a.b.long",
                        expRecords03, expDecimalType, false, null },
                // int(-2147483648) - long(-8) = int(-2147483640)
                new Object[] { 0, new Long( -8 ), "no", expRecords04,
                        expIntType, false, expIntTypeToJava } };
    }

    private String clName = "subtract_selector12585";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}",
                "{arr:[{'$numberLong':'9223372036854775807'},-2100],obj:{a:{b:{int:2147483647,long:{'$numberLong':'9223372036854775800'}}}},test:1}",
                "{arr:[1,0,[2147483147,1]],obj:{a:1},test:2}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSubtract( int matcherValue, Object subValue,
            String selectorName, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) throws Exception {

        BSONObject sValue = new BasicBSONObject();
        sValue.put( "$subtract", subValue );
        NumOverflowUtils.selectorOper( cl, matcherValue, sValue, selectorName,
                expRecords );
        NumOverflowUtils.checkDataType( cl, sValue, matcherValue, selectorName,
                expTypeToSdb, isVerifyTypeToJava, typeToJava );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

}
