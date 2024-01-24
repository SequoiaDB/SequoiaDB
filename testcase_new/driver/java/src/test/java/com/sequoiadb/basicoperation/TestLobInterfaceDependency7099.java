package com.sequoiadb.basicoperation;

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
 * FileName: TestLobInterfaceDependency7099.java test lob interfaces dependency:
 * 1.read lob after create lob,the result is fail 2.write lob after open lob,the
 * result is fail 3.write lob after seek lob,the result is fail
 * 
 * @author wuyan
 * @Date 2016.9.23
 * @version 1.00
 */
public class TestLobInterfaceDependency7099 extends SdbTestBase {

    private String clName = "cl_7099";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    String wMd5 = "";

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
     * write lob after open lob,the result is fail
     */
    // TODO:bug:SEQUOIADBMAINSTREAM-1988
    public void writeAfterOpenLob( String lobSb ) {
        DBLob lob = null;
        DBLob rLob = null;
        ObjectId oid = null;
        try {
            lob = cl.createLob();
            lob.write( lobSb.getBytes() );
            oid = lob.getID();
            lob.close();

            rLob = cl.openLob( oid );
            rLob.write( lobSb.getBytes() );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            Assert.assertEquals( -6, e.getErrorCode(),
                    e.getErrorCode() + e.getMessage() );
        } finally {
            if ( rLob != null ) {
                rLob.close();
            }
        }
    }

    /**
     * read lob after create lob,the result is fail
     */
    public void readAfterCreateLob( String lobSb ) {
        DBLob lob = null;
        try {
            lob = cl.createLob();
            byte[] buff = lobSb.getBytes();
            // Do not write lob directly read
            lob = cl.openLob( lob.getID() );
            lob.read( buff );
            lob.close();
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            // expected results to retrun to -317,then ignore exceptions
            Assert.assertEquals( e.getErrorCode(), -317,
                    e.getErrorCode() + e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }
    }

    /**
     * write lob after Seek lob,the result is fail
     */
    // TODO:bug:SEQUOIADBMAINSTREAM-1988
    public void writeAfterSeekLob( String lobSb ) {
        DBLob lob = null;
        byte[] buff = lobSb.getBytes();
        try {
            lob = cl.createLob();
            lob.write( buff );

            long pos = 0;
            lob.seek( pos, DBLob.SDB_LOB_SEEK_SET );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            // expected results to retrun to -32,then ignore exceptions
            Assert.assertEquals( -32, e.getErrorCode(),
                    e.getErrorCode() + e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
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
        // the generating buff length is 1024k
        String lobBytes = getRandomString( 1024 );
        readAfterCreateLob( lobBytes );
        // writeAfterSeekLob(lobBytes);
        writeAfterOpenLob( lobBytes );
    }

}
