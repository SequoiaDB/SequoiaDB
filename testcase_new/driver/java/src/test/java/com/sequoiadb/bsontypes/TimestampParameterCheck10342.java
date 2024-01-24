package com.sequoiadb.bsontypes;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
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
 * FileName: TimestampParameterCheck10342.java* test interface: BSONTimestamp
 * (int time, int inc),getTime () TestLink: seqDB-10342:time and inc parameters
 * check,verify the illegal value
 * 
 * @author wuyan
 * @Date 2016.10.13
 * @version 1.00
 */
public class TimestampParameterCheck10342 extends SdbTestBase {

    private String clName = "cl_10342";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        createCL();
    }

    private void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }

        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    /**
     * sequoiadb define time
     * range:1902-01-01-00.00.00.000000~2037-12-31-23.59.59.999999, convert
     * seconds is:-2145945600~2145887999
     */
    @DataProvider(name = "generateDataProvider")
    public Object[][] generateDataProvider() {
        return new Object[][] {
                // beyond left boundary:-2145945601
                { -2145945601, 0 },
                // beyond right boundary:
                { 2145888000, 999999 },
                // not int type
                { ( int ) 21345.12, ( int ) 999999.012 } };
    }

    /**
     * test beyond the timestamp of sequaoidb boundary value,number type not int
     * type after the actual value is inserted into the sequaoidb
     */
    @Test(dataProvider = "generateDataProvider")
    public void testBeyondBoundary( int seconds, int inc ) {
        try {
            BSONObject obj = new BasicBSONObject();
            BSONTimestamp timestamp = new BSONTimestamp( seconds, inc );
            obj.put( "timestamp", timestamp );
            cl.insert( obj );
            // check the insert result
            BSONObject tmp = new BasicBSONObject();
            DBCursor tmpCursor = cl.query( tmp, null, null, null );
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
            }
            tmpCursor.close();
            Assert.assertEquals( actRecs, obj,
                    "check datas are unequal\n" + "actDatas: " + actRecs + "\n"
                            + "expectDatas: " + timestamp.toString() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
        }
    }

    /**
     * test Illegal input,set the time and inc is null,expect error
     */
    // @Test
    /*
     * public void testIllegalType() { try{ BSONTimestamp timestamp = new
     * BSONTimestamp((Integer) null,(Integer) null);
     * Assert.fail("expect result need throw an error but not.");
     * }catch(NullPointerException e){ } }
     */

    @Test(expectedExceptions = IllegalArgumentException.class, expectedExceptionsMessageRegExp = "NullPoint")
    public void testIllegalType() {
        try {
            BSONTimestamp timestamp = new BSONTimestamp( ( Integer ) null,
                    ( Integer ) null );
        } catch ( NullPointerException e ) {
            throw new IllegalArgumentException( "NullPoint" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

}
