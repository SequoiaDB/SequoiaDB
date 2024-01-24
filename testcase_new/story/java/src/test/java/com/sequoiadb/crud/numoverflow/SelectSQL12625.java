package com.sequoiadb.crud.numoverflow;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: SelectSQL12625.java test content:StrictDataType=true,Numeric value
 * overflow for subcl , and execute sql in database. testlink case:seqDB-12625
 * 
 * @author luweikang
 * @Date 2017.9.18
 * @version 1.00
 */

public class SelectSQL12625 extends SdbTestBase {

    private String cs_name = "subcs_operators12625";
    private String clName = "subcl_operators12625";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;
    List< String > groupNameList = new ArrayList<>();

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {

        String[] expRecords1 = { "{'no':-2147484882}" };
        String[] expRecords2 = {
                "{'long':{'$numberLong':'-1657091634361260'}}" };
        String[] expRecords3 = { "{'no':124480 }" };
        String[] expRecords4 = { "{'no':126418.944 }" };
        String[] expRecords5 = { "{'long':-1.5372286795968E14}" };

        return new Object[][] {
                // the parameters: String sql
                new Object[] { "no - 1234", 2, expRecords1 },
                new Object[] { "long / 5566", 6, expRecords2 },
                new Object[] { "no + 123456", 3, expRecords3 },
                new Object[] { "no * 123.456", 7, expRecords4 },
                new Object[] { "long / (-0.3)", 13, expRecords5 } };
    }

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        if ( NumOverflowUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupNameList = NumOverflowUtils.getDataGroups( sdb );
        if ( groupNameList.size() < 2 ) {
            throw new SkipException( "groups less than 2 skip testcase" );
        }
        cs = this.createCS( cs_name );

        String[] records = {
                "{'no':-2147483648,'long':{'$numberLong':'-9223372036854775808'},'test':2}",
                "{'no':1024,'long':{'$numberLong':'46116860387904'},'test':3}",
                "{'no':-2147483648,'long':{'$numberLong':'-9223372036854775808'},'test':6}",
                "{'no':1024,'long':{'$numberLong':'46116860387904'},'test':7}",
                "{'no':-2147483648,'long':{'$numberLong':'-9223372036854775808'},'test':12}",
                "{'no':1024,'long':{'$numberLong':'46116860387904'},'test':13}" };
        cl = this.createSubCL( clName, "test", records );
    }

    @Test(dataProvider = "operData")
    public void testSQL( String condition, int mValue, String[] expRecords ) {
        String sql = "select " + condition + " from " + cl.getFullName()
                + " where test = " + mValue;
        try {
            NumOverflowUtils.sqlOper( sdb, sql, expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "many opertors data are used as selector oper failed,"
                            + e.getMessage() );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( cs_name ) ) {
                sdb.dropCollectionSpace( cs_name );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public CollectionSpace createCS( String csName ) {
        CollectionSpace cs = null;
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }

            cs = sdb.createCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cs;
    }

    public DBCollection createSubCL( String clName, String skey,
            String[] records ) {
        cl = NumOverflowUtils.createCL( cs, clName, "{ShardingKey:{'" + skey
                + "':1},ShardingType:'range',IsMainCL:true,StrictDataMode:true}" );
        String normalCL = clName + "_1";
        String rangeCL = clName + "_2";
        String hashCL = clName + "_3";

        // create CL
        DBCollection cl1 = NumOverflowUtils.createCL( cs, normalCL,
                "{StrictDataMode:true}" );
        DBCollection cl2 = NumOverflowUtils.createCL( cs, rangeCL,
                "{ShardingKey:{'test':1},ShardingType:'range',StrictDataMode:true,Group:'"
                        + groupNameList.get( 0 ) + "'}" );
        DBCollection cl3 = NumOverflowUtils.createCL( cs, hashCL,
                "{ShardingKey:{'test':1},ShardingType:'hash',StrictDataMode:true,Group:'"
                        + groupNameList.get( 0 ) + "'}" );

        // mainCL attach cl
        this.attachCL( cl, cl1.getFullName(), "test", 0, 5 );
        this.attachCL( cl, cl2.getFullName(), "test", 5, 10 );
        this.attachCL( cl, cl3.getFullName(), "test", 10, 15 );

        NumOverflowUtils.insert( cl, records );
        // split cl
        try {
            cl2.split( groupNameList.get( 0 ), groupNameList.get( 1 ), 50 );
            cl3.split( groupNameList.get( 0 ), groupNameList.get( 1 ), 50 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

        return cl;
    }

    public void attachCL( DBCollection clName, String attachCLFullName,
            String skey, Object lowBound, Object upBound ) {
        BSONObject low = new BasicBSONObject();
        BSONObject up = new BasicBSONObject();
        BSONObject option = new BasicBSONObject();
        low.put( skey, lowBound );
        up.put( skey, upBound );
        option.put( "LowBound", low );
        option.put( "UpBound", up );
        try {
            clName.attachCollection( attachCLFullName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

    }

}
