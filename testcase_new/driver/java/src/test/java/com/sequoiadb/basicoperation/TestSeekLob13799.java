package com.sequoiadb.basicoperation;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Random;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSeekLob13799.java test content:lob seek testlink
 * case:seqDB-13799
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */

public class TestSeekLob13799 extends SdbTestBase {

    private String clName = "cl_lob13799";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private Random random = new Random();

    @DataProvider(name = "pagesizeProvider")
    public Object[][] generatePageSize() {
        return new Object[][] {
                // seekType
                new Object[] { DBLob.SDB_LOB_SEEK_SET },
                new Object[] { DBLob.SDB_LOB_SEEK_CUR },
                new Object[] { DBLob.SDB_LOB_SEEK_END }, };
    }

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        createCL();
    }

    /**
     * put lob ,then change the read position of the lob to read
     * 
     * @param seektype
     *            SDB_LOB_SEEK_SET:the offset is relative to the start of the
     *            lob SDB_LOB_SEEK_CUR:the current position of lob
     *            SDB_LOB_SEEK_END:the end of lob
     */
    @Test(dataProvider = "pagesizeProvider")
    public void putLob( int seektype ) {
        int lobsize = random.nextInt( 1048576 );
        String lobSb = Commlib.getRandomString( lobsize );
        ObjectId oid = null;
        DBLob lob = null;

        // write lob
        try {
            lob = cl.createLob();
            lob.write( lobSb.getBytes() );

            oid = lob.getID();
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "write lob fail:" + e.getMessage() + e.getStackTrace() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }

        // set the seek position of the lob,the read lob
        DBLob rLob = null;
        try {
            rLob = cl.openLob( oid );

            int offset = 15;
            byte[] rbuff1 = new byte[ offset ];
            rLob.read( rbuff1 );
            ByteBuffer bbuff = ByteBuffer.allocate( offset );
            bbuff.put( rbuff1 );
            bbuff.rewind();
            String md51 = getMd5( bbuff );

            long pos = 0;
            if ( seektype == DBLob.SDB_LOB_SEEK_SET ) {
                pos = 0;
            } else if ( seektype == DBLob.SDB_LOB_SEEK_CUR ) {
                pos = -15;
            } else if ( seektype == DBLob.SDB_LOB_SEEK_SET ) {
                pos = rLob.getSize();
            }
            rLob.seek( pos, seektype );
            rLob.read( rbuff1 );
            bbuff = ByteBuffer.allocate( offset );
            bbuff.put( rbuff1 );
            bbuff.rewind();

            String md52 = getMd5( bbuff );
            Assert.assertEquals( md51, md52 );
            rLob.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "seek lob failed:" + e.getMessage() + e.getErrorCode() );
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
        } finally {
            if ( null != sdb ) {
                sdb.disconnect();
            }
        }
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

    private static String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
    }

}
