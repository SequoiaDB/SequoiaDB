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
 * FileName: SelectSQL12621.java test content:Numeric value overflow for single
 * character,and execute sql in database. testlink case:seqDB-12621
 * 
 * @author luweikang
 * @Date 2017.9.15
 * @version 1.00
 */
public class SelectSQL12621 extends SdbTestBase {
    private String clName = "sql_select12621";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = { "{'no':-2147484882}" };
        String[] expRecords2 = {
                "{'long':{'$numberLong':'-1657091634361260'}}" };
        String[] expRecords3 = { "{'no':124480 }" };
        String[] expRecords4 = { "{'no':126418.944 }" };
        String[] expRecords5 = { "{'long':-1.5372286795968E14}" };

        return new Object[][] {
                // the parameters: sql,expRecords
                new Object[] { "no - 1234 ", 1, expRecords1 },
                new Object[] { "long / 5566 ", 1, expRecords2 },
                new Object[] { "no + 123456 ", 2, expRecords3 },
                new Object[] { "no * 123.456 ", 2, expRecords4 },
                new Object[] { "long / (-0.3) ", 2, expRecords5 }, };
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
                "{'no':1024,'long':{'$numberLong':'46116860387904'},'test':2}" };

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
