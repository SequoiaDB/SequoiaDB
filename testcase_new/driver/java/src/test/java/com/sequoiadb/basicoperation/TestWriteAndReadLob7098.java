package com.sequoiadb.basicoperation;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
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
 * FileName: TestWriteAndReadLob7098.java test interface: write (byte[] b, int
 * off, int len) read (byte[] b, int off, int len)
 * 
 * @author wuyan
 * @Date 2016.9.23
 * @version 1.00
 */
public class TestWriteAndReadLob7098 extends SdbTestBase {

    private String clName = "cl_7098";
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

    public static String getMd5( Object inbuff ) {
        MessageDigest md5;
        String value = "";
        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else {
                Assert.fail( "invalid parameter!" );
            }

            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return value;
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
     * test write (byte[] b, int off, int len)
     * 
     * @return oid : lob oid
     */
    public ObjectId writeLob( String lobSb ) {
        DBLob lob = null;
        ObjectId oid = null;
        byte[] buff = lobSb.getBytes();
        int off = 0;
        int writeLen = 0;
        int len = buff.length;
        try {
            lob = cl.createLob();
            while ( len > 0 ) {
                writeLen = len > 512 ? 512 : len;
                lob.write( buff, off, writeLen );
                off += writeLen;
                len -= writeLen;
            }

            oid = lob.getID();
            ByteBuffer wbuff = ByteBuffer.allocate( buff.length );
            wbuff.put( buff );
            wbuff.rewind();
            wMd5 = getMd5( wbuff );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "write lob fail:" + e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }
        return oid;
    }

    /**
     * test write (byte[] b, int off, int len) test interface return
     */
    public void readLob( ObjectId oid, String lobSb ) {
        DBLob rLob = null;
        try {
            rLob = cl.openLob( oid );
            int off = 0;
            byte[] buff = lobSb.getBytes();
            int totalLen = buff.length;
            int len = buff.length > 512 ? 512 : buff.length;
            int readLen = 0;
            do {
                // check return value is the total number of bytes read into the
                // buffer
                readLen = rLob.read( buff, off, len );
                off += readLen;
                totalLen -= readLen;
                len = totalLen > 512 ? 512 : totalLen;
            } while ( readLen > 0 && totalLen > 0 );

            // check write lob and read lob md5
            ByteBuffer rbuff = ByteBuffer.allocate( buff.length );
            rbuff.put( buff );
            rbuff.rewind();
            String rMd5 = getMd5( rbuff );
            Assert.assertEquals( wMd5, rMd5,
                    "the write an read file md5 different" );

            // check the return value: -1 if the file has been reached
            Assert.assertEquals( rLob.read( buff, 0, buff.length ), -1,
                    "the return must be 1" );
            // check the return value:0 if len is Zero.
            String lobSb1 = getRandomString( 0 );
            byte[] buff1 = lobSb1.getBytes();
            Assert.assertEquals( rLob.read( buff1, 0, 0 ), 0,
                    "the return must be 0" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "read lob fail " + e.getMessage() );
        } finally {
            if ( rLob != null ) {
                rLob.close();
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
    public void testLob7098() {
        // the generating buff length is 1024k
        String sb = getRandomString( 1024 );
        ObjectId oid = writeLob( sb );
        readLob( oid, sb );
    }

}
