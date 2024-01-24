package com.sequoiadb.basicoperation;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestLob7097.java test interface: createLob (ObjectId id);
 * getCreateTime () getID()/removeLob(ObjectId lobID)/close ()--the same as
 * testcase:testReadAndRemoveLobs7843 getSize()/openLob (ObjectId id);--the same
 * as testcase:testSeekLob7839(seqDB-7839)
 * 
 * @author wuyan
 * @Date 2016.9.22
 * @version 1.00
 */
public class TestLob7097 extends SdbTestBase {

    private String clName = "cl_7097";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    SimpleDateFormat sdf = new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" );

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
     * generating byte to write lob
     * 
     * @param length
     *            generating byte stream size
     * @return String sb
     */
    public static String getRandomString( int length ) {
        String base = "abcdefahijklmnopqrstuvwxyz0123456789";
        Random random = new Random();

        int times = length / 1024;
        if ( times < 1 ) {
            times = 1;
        }

        int len = length >= 1024 ? 1024 : 0;
        int mulByteNum = length % 1024;

        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < times; ++i ) {
            if ( i == times - 1 ) {
                len += mulByteNum;
            }

            char ch = base.charAt( random.nextInt( base.length() ) );
            for ( int k = 0; k < len; ++k ) {
                sb.append( ch );
            }
        }
        return sb.toString();
    }

    /**
     * test createLob (ObjectId id); getID (); getCreateTime ()
     */
    public void putLob( String lobSb ) {
        DBLob lob = null;
        ObjectId oid = null;
        try {
            ObjectId id = new ObjectId();
            lob = cl.createLob( id );
            lob.write( lobSb.getBytes() );
            oid = lob.getID();
            Assert.assertEquals( id, oid,
                    "designated id must same as return oid" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "write lob fail:" + e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }
        try {
            DBLob rLob = cl.openLob( oid );
            // test getCreateTime
            long time = rLob.getCreateTime();
            long currentTime = new Date().getTime();
            long result = Math.abs( ( time - currentTime ) / ( 1000 * 36 ) );
            // the time interval does not exceed 60 minutes
            if ( result > 60 ) {
                Assert.assertTrue( false,
                        "the create time:" + sdf.format( new Date( time ) )
                                + "\n" + "the currentTime:" + new Date() );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
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

    @Test
    public void testLob() {
        String sb = getRandomString( 1024 );
        putLob( sb );
    }

}
