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
 * FileName: AdsIsSelector12570.java test content:Numeric value overflow for
 * single character using $abs operation, and the $abs is used as a selector.
 * testlink case:seqDB-12570
 * 
 * @author wuyan
 * @Date 2017.9.1
 * @version 1.00
 */

public class AdsIsSelector12570 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        String[] expRecords1 = {
                "{'no':2147483648,'tlong':-9223372036854775808,'tdouble':-1.7E+308,'test':0}" };
        String[] expRecords2 = {
                "{'no':-2147483648,'tlong':{ '$decimal' : '9223372036854775808'},'tdouble':-1.7E+308,'test':0}" };
        String[] expRecords3 = {
                "{'no':-2147483648,'tlong':-9223372036854775808,'tdouble':1.7E+308,'test':0}" };
        String[] expRecords4 = {
                "{no:1,numarry:[2147483648],obj:{a:-2147483648},test:1}" };
        String[] expRecords5 = {
                "{no:1,numarry:[2147483648,{ '$decimal' : '9223372036854775808'} , null],obj:{a:-2147483648},test:1}" };
        String[] expRecords6 = {
                "{no:2147483647,obj:{a:{b:{'$decimal':'9223372036854775808'}}},test:3}" };
        String expLongType = "int64";
        String expLongTypeToJava = "class java.lang.Long";
        String expDecimalType = "decimal";
        String expArrayType = "array";
        String expDoubleType = "double";
        String expDecimalTypeToJava = "class org.bson.types.BSONDecimal";
        String expDoubleTypeToJava = "class java.lang.Double";

        return new Object[][] {
                // parameters:
                // matcherValue,selectorName,expRecords,expType,isVerifyTypeToJava,
                // expTypeTojava
                // test int32 type numberflow
                new Object[] { 0, "no", expRecords1, expLongType, true,
                        expLongTypeToJava },
                // test int64 type numberflow
                new Object[] { 0, "tlong", expRecords2, expDecimalType, true,
                        expDecimalTypeToJava },
                // test double type
                new Object[] { 0, "tdouble", expRecords3, expDoubleType, true,
                        expDoubleTypeToJava },
                // the arr.0 use $abs,the numberflow is int32 To int64
                new Object[] { 1, "numarry.$[0]", expRecords4, expLongType,
                        false, null },
                // the arr use $abs
                new Object[] { 1, "numarry", expRecords5, expArrayType, false,
                        null },
                // test obj type use $abs
                new Object[] { 3, "obj.a.b", expRecords6, expDecimalType, false,
                        null }, };
    }

    private String clName = "abs_selector12570";
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

        String clOption = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'tdouble':-1.7E+308,'test':0}",
                "{no:1,numarry:[-2147483648,{'$numberLong':'-9223372036854775808'},'testo1'],obj:{a:-2147483648},test:1}",
                "{no:2147483647,obj:{a:{b:{'$numberLong':'-9223372036854775808'}}},test:3}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testAbs( int matcherValue, String selectorName,
            String[] expRecords, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String typeToJava ) {
        try {
            BSONObject sValue = new BasicBSONObject();
            sValue.put( "$abs", 1 );
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
