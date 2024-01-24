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
 * @Description seqDB-12580:使用$multiply操作不同类型运算
 * @author luweikang
 * @Date 2017.9.11
 * @updateUser wuyan
 * @updateDate 2021.9.3
 * @updateRemark 新增int32类型数值运算不溢出情况下保持原类型测试单
 * @version 1.00
 */

public class MultiplyIsSelector12580 extends SdbTestBase {

    private String clName = "multiply_selector12580";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = {
                "{'no':2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        String[] expRecords2 = {
                "{'no':-2147483648,'tlong':{'$decimal':'9223372036854775808'},'test':0}" };
        String[] expRecords3 = {
                "{'no':2147483647,'long':{'$decimal':'-18446744073709551614'},test:1}" };
        String[] expRecords4 = { "{'no':246.0,'double':123.5,'test':2}" };
        String[] expRecords5 = { "{'no':123.0,'double':247.0,'test':2}" };
        String[] expRecords6 = {
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'arr':[2147483648],'obj':{a:'123'},'test':3}" };
        String[] expRecords7 = {
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'arr':[1000000000,-2147483648],'obj':null,'test':3}" };
        String[] expRecords8 = { "{'no':2147483646,'double':2.3,'test':5}" };
        String expJavaLong = "class java.lang.Long";
        String expJavaDouble = "class java.lang.Double";
        String expJavaDecimal = "class org.bson.types.BSONDecimal";
        String expJavaInt = "class java.lang.Integer";
        String expLongType = "int64";
        String expDoubleType = "double";
        String expDecimalType = "decimal";
        String expIntType = "int32";

        return new Object[][] {
                // the parameters:
                // matcherValue,subValue,selectorName,expRecords,expType,isVerifyDataType,expTypeToJava
                // -2147483648 $multiply -1 the result is 2147483648(int64)
                new Object[] { 0, new Integer( -1 ), "no", expRecords1,
                        expLongType, true, expJavaLong },
                // -9223372036854775808 $multiply -1 the result is
                // 9223372036854775808(decimal)
                new Object[] { 0, new Integer( -1 ), "tlong", expRecords2,
                        expDecimalType, true, expJavaDecimal },
                // 9223372036854775807 $multiply -2 the result is
                // {'$decimal':'-18446744073709551614'}
                new Object[] { 1, new Long( -2 ), "long", expRecords3,
                        expDecimalType, true, expJavaDecimal },
                // 123.0 $multiply 2 the result is 246.0
                new Object[] { 2, new Integer( 2 ), "no", expRecords4,
                        expDoubleType, true, expJavaDouble },
                // 123.5 $multiply 2(int64) the result is 247.0
                new Object[] { 2, new Integer( 2 ), "double", expRecords5,
                        expDoubleType, true, expJavaDouble },
                // arr $multiply -1 the result is [2147483648]
                new Object[] { 3, new Integer( -1 ), "arr.$[1]", expRecords6,
                        expLongType, false, expJavaLong },
                // obj $multiply 1 the result is null
                new Object[] { 3, new Integer( -1 ), "obj", expRecords7, "null",
                        false, null },
                // int(1073741823) type $multiply numberLong(2) the result is
                // int(2147483646)
                new Object[] { 5, new Long( 2 ), "no", expRecords8, expIntType,
                        false, expJavaInt } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}",
                "{'no':2147483647,'long':{'$numberLong':'9223372036854775807'},'test':1}",
                "{'no':123.0,'double':123.5,'test':2}",
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'arr':[1000000000,-2147483648],'obj':{a:'123'},'test':3}",
                "{'no':1073741823,'double':2.3,'test':5}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testMultiply( int matcherValue, Object mulValue,
            String selectorName, String[] expRecords, String expType,
            Boolean isVerifyTypeToJava, String expTypeToJava )
                    throws Exception {

        BSONObject mValue = new BasicBSONObject();
        mValue.put( "$multiply", mulValue );
        NumOverflowUtils.selectorOper( cl, matcherValue, mValue, selectorName,
                expRecords );

        NumOverflowUtils.checkDataType( cl, mValue, matcherValue, selectorName,
                expType, isVerifyTypeToJava, expTypeToJava );

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
