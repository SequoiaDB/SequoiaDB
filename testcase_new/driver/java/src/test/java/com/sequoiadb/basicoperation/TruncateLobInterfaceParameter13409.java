package com.sequoiadb.basicoperation;

import java.util.Date;

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
 * FileName: TruncateLobInterfaceParameter13409.java test content:truncate lob
 * interface parameter checking,test interface:truncateLob() testlink
 * case:seqDB-13409
 * 
 * @author wuyan
 * @Date 2017.11.23
 * @version 1.00
 */
public class TruncateLobInterfaceParameter13409 extends SdbTestBase {

    private String clName = "writelob13409";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private ObjectId oid = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );

    }

    @Test
    public void testLob() {
        int writeLobSize = 1024 * 256;
        byte[] lobBuff = Commlib.getRandomBytes( writeLobSize );
        oid = Commlib.createAndWriteLob( cl, lobBuff );

        // test legal parameters:oid/length
        // 1.the length is maxlongvalue
        long truncatelength = 9223372036854775807L;
        cl.truncateLob( oid, truncatelength );
        byte[] readLobBuff = readLobBuff( cl, oid );
        Commlib.assertByteArrayEqual( readLobBuff, lobBuff );

        // 2.test the lobsize is 0;
        long truncatelength1 = 0;
        cl.truncateLob( oid, truncatelength1 );
        byte[] readLobBuff1 = readLobBuff( cl, oid );
        byte[] expBuff0 = new byte[ 0 ];
        Commlib.assertByteArrayEqual( readLobBuff1, expBuff0 );

        // illegal parameter test
        // 1.the truncatelength is -1
        try {
            long errorlength1 = -1;
            cl.truncateLob( oid, errorlength1 );
        } catch ( BaseException e ) {
            // expected results to retrun to -6,then ignore exceptions
            Assert.assertEquals( -6, e.getErrorCode(),
                    e.getErrorCode() + e.getMessage() );
        }

        // 2.the oid is error
        try {
            ObjectId errorOid = new ObjectId( "5a2e20cca298606b44000000" );
            long errorlength2 = 1;
            cl.truncateLob( errorOid, errorlength2 );
        } catch ( BaseException e ) {
            // expected results to retrun to -4,then ignore exceptions
            Assert.assertEquals( -4, e.getErrorCode(),
                    e.getErrorCode() + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }

    }

    private byte[] readLobBuff( DBCollection cl, ObjectId oid ) {
        byte[] rbuff = null;
        try ( DBLob rlob = cl.openLob( oid )) {
            rbuff = new byte[ ( int ) rlob.getSize() ];
            rlob.read( rbuff );
        }
        return rbuff;
    }
}
