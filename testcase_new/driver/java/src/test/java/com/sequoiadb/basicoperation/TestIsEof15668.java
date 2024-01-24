package com.sequoiadb.basicoperation;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
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
 * FileName: testIsEof15668.java test content:test implement isEof testlink
 * case:seqDB-15668
 * 
 * @author wangkexin
 * @Date 2018.08.27
 * @version 1.00
 */
public class TestIsEof15668 extends SdbTestBase {
    private String clName = "cl_lob15668";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private static ObjectId oid = null;
    private String prevMd5 = "";
    private Random random = new Random();

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

    @Test
    public void testSplitAndWrite() {
        putLob();
        readLob();
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void readLob() {
        try ( Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            String curMd5 = "";
            DBLob rLob = null;
            rLob = cl2.openLob( oid );
            byte[] rbuff = new byte[ 1024 ];
            int readLen = 0;
            ByteBuffer bytebuff = ByteBuffer.allocate( ( int ) rLob.getSize() );

            // flag is true when read operation not completed
            while ( ( readLen = rLob.read( rbuff ) ) != -1 ) {
                bytebuff.put( rbuff, 0, readLen );
            }
            bytebuff.rewind();
            rLob.close();
            if ( rLob.isEof() ) {
                curMd5 = getMd5( bytebuff );
                Assert.assertEquals( curMd5, prevMd5,
                        "the lobs md5 different" );
            } else {
                Assert.assertTrue( false,
                        "implement isEof() failed when already finish read" );
            }
        } catch ( BaseException e ) {
            if ( -4 != e.getErrorCode() && -317 != e.getErrorCode()
                    && -268 != e.getErrorCode() && -269 != e.getErrorCode() ) {
                Assert.assertTrue( false,
                        "removeLob fail:" + e.getMessage() + e.getErrorCode() );
            }
        }
    }

    private void putLob() {
        int lobsize = random.nextInt( 1048576 );
        String lobSb = Commlib.getRandomString( lobsize );
        DBLob lob = null;
        try {
            lob = cl.createLob();
            lob.write( lobSb.getBytes() );
            oid = lob.getID();
            prevMd5 = getMd5( lobSb );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "write lob fail" + e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }
    }

    private void createCL() {
        try {
            String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );

            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    private String getMd5( Object inbuff ) {
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
