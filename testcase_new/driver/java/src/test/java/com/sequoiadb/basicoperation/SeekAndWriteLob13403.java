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
 * FileName: SeekAndWriteLob13403.java test content:seek to write lob of create
 * only mode ,than read lob of read mode testlink case:seqDB-13403
 * 
 * @author wuyan
 * @Date 2017.11.23
 * @version 1.00
 */
public class SeekAndWriteLob13403 extends SdbTestBase {
    private String clName = "writelob13403";
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
        int writeLobSize = 1024 * 200;
        int offset = 1024 * 2;
        testLobBuff = Commlib.getRandomBytes( writeLobSize );
        ObjectId oid = seekAndWriteLob( testLobBuff, offset );

        int rewriteOffset = 1024 * 3;
        byte[] rewriteBuff = Commlib.getRandomBytes( writeLobSize );
        seekAndRewriteLob( oid, rewriteBuff, rewriteOffset );
        readAndcheckResult( oid, rewriteBuff, offset, rewriteOffset );
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

    private ObjectId seekAndWriteLob( byte[] writeBuff, int offset ) {
        ObjectId oid = null;
        try ( DBLob lob = cl.createLob()) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( writeBuff );
            oid = lob.getID();
        }
        return oid;
    }

    private void seekAndRewriteLob( ObjectId oid, byte[] writeBuff,
            int offset ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( writeBuff );
        }
    }

    private void readAndcheckResult( ObjectId oid, byte[] rewriteBuff,
            int offset, int rewriteoffset ) {
        testLobBuff = Commlib.appendBuff( testLobBuff, rewriteBuff,
                rewriteoffset - offset );
        byte[] readLobBuff = new byte[ testLobBuff.length ];
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.read( readLobBuff );
        }

        // check the all write lob
        Commlib.assertByteArrayEqual( readLobBuff, testLobBuff );
    }
}
