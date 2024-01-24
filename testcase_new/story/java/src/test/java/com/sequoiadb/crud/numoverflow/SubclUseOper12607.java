package com.sequoiadb.crud.numoverflow;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
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
 * FileName: SubclUseOper12607.java test content:Numeric value overflow for
 * subcl using operators, and these operators are used as a selector. testlink
 * case:seqDB-12607
 * 
 * @author luweikang
 * @Date 2017.9.14
 * @version 1.00
 */

public class SubclUseOper12607 extends SdbTestBase {

    private String cs_name = "subcs_operators12607";
    private String clName = "subcl_operators12607";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;
    List< String > groupNameList = new ArrayList<>();

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = { "{'no':-2147483648,'test':3}" };
        String[] expRecords2 = {
                "{'no':{'$numberLong':'9223372036854775807'},'test':7}" };
        String[] expRecords3 = {
                "{'no':[2147483647,{'$numberLong':'-9223372036854775808'}],'test':8}" };
        String[] expRecords4 = {
                "{'no':[2147483647,[-21474836478]],'test':12}" };
        String[] expRecords5 = {
                "{'no':{a:{b:{'$numberLong':'9223372036854775807'}}},'test':13}" };
        String[] expRecords6 = {
                "{'no':[2147483647,{'$numberLong':'-9223372036854775808'}],'test':8}",
                "{'no':[2147483647,[-21474836478]],'test':12}" };

        return new Object[][] {
                // the parameters: String opertors, Object operValue, String
                // matcherName, String[] expRecords
                new Object[] { "$add", new Integer( -1 ),
                        new Long( -2147483649L ), "no", expRecords1 },
                new Object[] { "$subtract", new Integer( -1 ),
                        new BigDecimal( "9223372036854775808" ), "no",
                        expRecords2 },
                new Object[] { "$multiply", new Long( -1 ),
                        new BigDecimal( "9223372036854775808" ), "no.1",
                        expRecords3 },
                new Object[] { "$divide", new Long( -1 ),
                        new Long( 21474836478L ), "no.1.0", expRecords4 },
                new Object[] { "$multiply", new BigDecimal( 2 ),
                        new BigDecimal( "18446744073709551614" ), "no.a.b",
                        expRecords5 },
                new Object[] { "$add", new Long( 2147483647 ),
                        new Long( 4294967294L ), "no.0", expRecords6 },
                new Object[] { "$divide", new Integer( 334455 ),
                        new Long( 27577318434033L ), "no", expRecords2 } };
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

        String[] records = { "{'no':-2147483648,'test':3}",
                "{'no':{'$numberLong':'9223372036854775807'},'test':7}",
                "{'no':[2147483647,{'$numberLong':'-9223372036854775808'}],'test':8}",
                "{'no':[2147483647,[-21474836478]],'test':12}",
                "{'no':{a:{b:{'$numberLong':'9223372036854775807'}}},'test':13}", };
        cl = this.createSubCL( clName, "test", records );
    }

    @Test(dataProvider = "operData")
    public void testSubclOper( String opertors, Object operValue,
            Object resultValue, String matcherName, String[] expRecords ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            mValue.put( opertors, operValue );
            mValue.put( "$et", resultValue );
            NumOverflowUtils.matcherOper( cl, matcherName, mValue, expRecords );

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
                + "':1},ShardingType:'range',IsMainCL:true}" );
        String normalCL = clName + "_1";
        String rangeCL = clName + "_2";
        String hashCL = clName + "_3";

        // create CL
        DBCollection cl1 = NumOverflowUtils.createCL( cs, normalCL );
        DBCollection cl2 = NumOverflowUtils.createCL( cs, rangeCL,
                "{ShardingKey:{'test':1},ShardingType:'range',Group:'"
                        + groupNameList.get( 0 ) + "'}" );
        DBCollection cl3 = NumOverflowUtils.createCL( cs, hashCL,
                "{ShardingKey:{'test':1},ShardingType:'hash',Group:'"
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
