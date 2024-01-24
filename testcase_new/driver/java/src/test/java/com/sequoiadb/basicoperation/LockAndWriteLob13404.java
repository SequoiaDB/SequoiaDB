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
 * FileName: LockAndWriteLob13404.java test content:lock to write lob ,test the
 * interface lock() and lockAndSeek() testlink case:seqDB-13404
 * 
 * @author wuyan
 * @Date 2017.11.23
 * @version 1.00
 */
public class LockAndWriteLob13404 extends SdbTestBase {
    private String clName = "writelob13404";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    byte[] testLobBuff = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }
    }

    @Test
    public void testLob() {
        int writeSize = 1024 * 2;
        testLobBuff = Commlib.getRandomBytes( writeSize );
        ObjectId oid = Commlib.createAndWriteLob( cl, testLobBuff );

        int writeLobSize = 1024 * 256;
        int offset = 1024 * 2;
        byte[] rewriteBuff = Commlib.getRandomBytes( writeLobSize );
        lockAndWriteLob( oid, rewriteBuff, offset );

        int rewriteOffset = 1024 * 256;
        lockAndseekToRewriteLob( oid, rewriteBuff, rewriteOffset );
        readAndcheckResult( oid );
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

    private void lockAndWriteLob( ObjectId oid, byte[] writeBuff, int offset ) {
        testLobBuff = Commlib.appendBuff( testLobBuff, writeBuff, offset );
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lock( offset, writeBuff.length );
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( writeBuff );
        }
    }

    private void lockAndseekToRewriteLob( ObjectId oid, byte[] writeBuff,
            int offset ) {
        testLobBuff = Commlib.appendBuff( testLobBuff, writeBuff, offset );
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lockAndSeek( offset, writeBuff.length );
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( writeBuff );
        }

    }

    private void readAndcheckResult( ObjectId oid ) {
        byte[] readLobBuff = new byte[ testLobBuff.length ];
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
            lob.read( readLobBuff );
        }
        // check the all write lob
        Assert.assertEquals( readLobBuff, testLobBuff );
    }
}
