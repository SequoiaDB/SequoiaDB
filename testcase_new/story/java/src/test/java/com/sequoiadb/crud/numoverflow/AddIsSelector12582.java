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
 * @Description: seqDB-12582:单个字段使用$add操作不同类型运算溢出 *
 * @author luweikang
 * @Date 2017.9.12
 * @updateUser wuyan
 * @updateDate 2021.8.14
 * @version 1.00
 */

public class AddIsSelector12582 extends SdbTestBase {

    private String clName = "add_selector12582";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = {
                "{'no':-2147483649,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}" };
        String[] expRecords2 = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-1'},'test':0}" };
        String[] expRecords3 = {
                "{'no':2147483647,'long':{'$decimal':'9223372036854775808'},'test':1}" };
        String[] expRecords4 = {
                "{'no':2147483646,'long':{'$numberLong':'9223372036854775807'},'test':1}" };
        String[] expRecords5 = { "{'no':[2147483147.5],'test':2}" };
        String[] expRecords6 = {
                "{'no':[{'$numberLong':'8223372036854775808'}],'test':2}" };
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
                new Object[] { 0, new Integer( -1 ), "no", expRecords1,
                        expLongType, true, expJavaLong },
                new Object[] { 0, new Long( 9223372036854775807L ), "tlong",
                        expRecords2, expLongType, true, expJavaLong },
                new Object[] { 1, new Integer( 1 ), "long", expRecords3,
                        expDecimalType, true, expJavaDecimal },
                new Object[] { 1, new Long( -1 ), "no", expRecords4, expIntType,
                        true, expJavaInt },
                new Object[] { 2, new Double( 0.5 ), "no.$[0]", expRecords5,
                        expDoubleType, false, expJavaDouble },
                new Object[] { 2, new Integer( 1 ), "no.$[1]", expRecords6,
                        expLongType, false, expJavaLong }, };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'test':0}",
                "{'no':2147483647,'long':{'$numberLong':'9223372036854775807'},'test':1}",
                "{'no':[2147483147,{'$numberLong':'8223372036854775807'}],'test':2}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testAdd( int matcherValue, Object mulValue, String selectorName,
            String[] expRecords, String expType, Boolean isVerifyTypeToJava,
            String expTypeToJava ) throws Exception {

        BSONObject mValue = new BasicBSONObject();
        mValue.put( "$add", mulValue );
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
