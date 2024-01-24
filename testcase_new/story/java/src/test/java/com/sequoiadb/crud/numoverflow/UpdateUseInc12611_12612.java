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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-12611:使用$inc更新不同类型数值运算溢出 ；seqDB-12612:使用$inc更新不同类型数值运算 *
 * @author luweikang
 * @Date 2017.9.15
 * @updateUser wuyan
 * @updateDate 2021.9.3
 * @updateRemark 新增int32类型数值运算不溢出情况下保持原类型测试单
 * @version 1.00
 */

public class UpdateUseInc12611_12612 extends SdbTestBase {

    private String clName = "inc_update12611_12612";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = {
                "{'no':-2147493648,'long':{'$numberLong':'-9223372036854775808'},'test':1}" };
        String[] expRecords2 = {
                "{'no':{'$numberLong':'-1073746824'},'long':{'$numberLong':'-9223372036854775808'},'test':1}" };
        String[] expRecords3 = {
                "{'no':{'$numberLong':'-1073746824'},'long':{'$decimal':'-9223372037928522632'},'test':1}" };
        String[] expRecords4 = {
                "{'no':{a:{b:2147493647}},'string':'123','test':2}" };
        // String[] expRecords5 = {
        // "{'no':[-2147483648,[3221225471],{'$numberLong':'9223372036854775807'}],test:3}"
        // };
        // String[] expRecords6 = {
        // "{'no':[-2147483648,[3221225471],{'$decimal':'9223372036854775808'}],test:3}"
        // };
        String[] expRecords7 = {
                "{'no':2147483646.5,'long':9223372036854775807,'test':4}" };
        String[] expRecords8 = {
                "{'no':2147483646.5,'long':9223372036854775806.5,'test':4}" };
        String[] expRecords9 = {
                "{'no':-2147483649,'long':-9223372036854775808,'test':5}" };
        String[] expRecords10 = {
                "{'no':-2147483649,'long':{$decimal:'-9223372036854775809'},'test':5}" };
        String[] expRecords11 = {
                "{'no':-2147483647,'long':{'$numberLong':'-333333333808'},'test':6}" };
        String[] expRecords12 = {
                "{'no':{a:{b:2147483646}},'long':{'$numberLong':'-8'},'test':7}" };

        String expJavaLong = "class java.lang.Long";
        String expJavaDouble = "class java.lang.Double";
        String expJavaDecimal = "class org.bson.types.BSONDecimal";
        String expJavaInt = "class java.lang.Integer";
        String expLongType = "int64";
        String expDoubleType = "double";
        String expDecimalType = "decimal";
        String expIntType = "int32";
        return new Object[][] {
                // the parameters: int matcherValue,String updateName, String
                // updateValue, String []expRecords,String expTypeToSdb,Boolean
                // isVerifyTypeToJava, String expTypeToJava
                new Object[] { 1, "no", new Long( -10000 ), expRecords1,
                        expLongType, true, expJavaLong },
                new Object[] { 1, "no", new Long( 1073746824 ), expRecords2,
                        expLongType, true, expJavaLong },
                new Object[] { 1, "long", new Integer( -1073746824 ),
                        expRecords3, expDecimalType, true, expJavaDecimal },
                new Object[] { 2, "no.a.b", new Long( 1073751824 ), expRecords4,
                        expLongType, false, null },

                // SEQUOIADBMAINSTREAM-2795
                // new Object[]{ 3, "no.1.0",new
                // Long(1073741824),expRecords5,expLongType,false,null},
                // new Object[]{ 3, "no.2",new
                // Integer(1),expRecords6,expDecimalType,false,null},
                new Object[] { 4, "no", new Double( -0.5 ), expRecords7,
                        expDoubleType, true, expJavaDouble },
                new Object[] { 4, "long", new Double( -0.5 ), expRecords8,
                        expDoubleType, true, expJavaDouble },
                new Object[] { 5, "no", new Integer( -1 ), expRecords9,
                        expLongType, true, expJavaLong },
                new Object[] { 5, "long", new Integer( -1 ), expRecords10,
                        expDecimalType, true, expJavaDecimal },
                // int(-2147483648) + long(1)= int(-2147483647)
                new Object[] { 6, "no", new Long( 1 ), expRecords11, expIntType,
                        true, expJavaInt },
                // int(2147483647) + long(-1)= int(2147483646)
                new Object[] { 7, "no.a.b", new Long( -1 ), expRecords12,
                        expIntType, false, null } };
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
                "{'no':-2147483648,'long':{'$numberLong':'-9223372036854775808'},'test':1}",
                "{'no':{a:{b:1073741823}},'string':'123','test':2}",
                "{'no':[-2147483648,[2147483647],{'$numberLong':'9223372036854775807'}],test:3}",
                "{'no':2147483647,'long':{'$numberLong':'9223372036854775807'},'test':4}",
                "{'no':-2147483648,'long':{'$numberLong':'-9223372036854775808'},'test':5}",
                "{'no':-2147483648,'long':{'$numberLong':'-333333333808'},'test':6}",
                "{'no':{a:{b:2147483647}},'long':{'$numberLong':'-8'},'test':7}", };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testInc( int matcherValue, String updateName,
            Object updateValue, String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String expTypeToJava )
                    throws Exception {

        BSONObject uValue = new BasicBSONObject();
        uValue.put( updateName, updateValue );
        NumOverflowUtils.updateOper( cl, matcherValue, uValue, "update" );
        NumOverflowUtils.checkUpdateResult( cl, matcherValue, expRecords );

        NumOverflowUtils.checkUpdateDataType( cl, matcherValue, updateName,
                expTypeToSdb, isVerifyTypeToJava, expTypeToJava );

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
