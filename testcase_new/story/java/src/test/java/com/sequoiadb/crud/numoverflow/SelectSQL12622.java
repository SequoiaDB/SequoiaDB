package com.sequoiadb.crud.numoverflow;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
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
 * FileName: SelectSQL12622.java test content:Numeric value overflow for single
 * character,and execute sql in database. testlink case:seqDB-12622
 * 
 * @author luweikang
 * @Date 2017.9.15
 * @version 1.00
 */
public class SelectSQL12622 extends SdbTestBase {
    private String clName = "sql_select12622";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = { "{'no':8589930496}" };
        String[] expRecords2 = {
                "{'long':{$decimal:'-18446744073709551616'} }" };
        String[] expRecords3 = { "{'no':{$numberLong:'414272144277504'}}" };
        String[] expRecords4 = {
                "{'long':{$decimal:'99035203580122776972318'} }" };

        return new Object[][] {
                // the parameters: matcherName,matcherValue,expRecords
                new Object[] { "(no + 1024)*(no + 2147483644)", 1,
                        expRecords1 },
                new Object[] {
                        "(long / 9223372036854775807) * (long / 4611686018427387904) * long",
                        1, expRecords2 },
                new Object[] { "(no / 5566) * (no - 1073741823)", 2,
                        expRecords3 },
                new Object[] { "(long * 2147483648) - (long / 2147483648)", 2,
                        expRecords4 } };
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
                "{'no':2147483647,'long':{'$numberLong':'46116860387904'},'test':2}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSQL( String condition, int mValue, String[] expRecords ) {
        String sql = "select " + condition + " from " + cl.getFullName()
                + " where test = " + mValue;
        try {
            this.checkData( sql, expRecords );

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "execute sql oper failed," + e.getMessage() );
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

    public void checkData( String sql, String[] expRecords ) {
        DBCursor cursor = sdb.exec( sql );
        List< BSONObject > actualList = new ArrayList< BSONObject >();

        while ( cursor.hasNext() ) {
            BSONObject object = cursor.getNext();
            actualList.add( object );
        }
        System.out.println( "actualList" + actualList.toString() );
        cursor.close();

        List< BSONObject > expectedList = new ArrayList< BSONObject >();
        for ( int i = 0; i < expRecords.length; i++ ) {
            BSONObject expRecord = ( BSONObject ) JSON.parse( expRecords[ i ] );
            expectedList.add( expRecord );
        }
        Assert.assertEquals( actualList, expectedList,
                "the actual query datas is error" );
    }
}
