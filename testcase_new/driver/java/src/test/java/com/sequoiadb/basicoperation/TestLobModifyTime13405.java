package com.sequoiadb.basicoperation;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: LockAndWriteLob13405.java test content:test the interface
 * getModificationTime() testlink case:seqDB-13405
 * 
 * @author wuyan
 * @Date 2017.11.23
 * @version 1.00
 */
public class TestLobModifyTime13405 extends SdbTestBase {
    private String clName = "writelob13405";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    ObjectId oid = new ObjectId( "5a0a6036ff644cd60a000000" );
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
        long lastModifyTime = putLob();

        int writeLobSize = 1024 * 256;
        int offset = 1024 * 3;
        byte[] rewriteBuff = Commlib.getRandomBytes( writeLobSize );
        lockAndWriteLob( oid, rewriteBuff, offset );

        getLastModifyTimeAndCheckResult( oid, lastModifyTime );
        checkModifyTimeAfterClose( oid );
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

    private long putLob() {
        int writeSize = 1024 * 2;
        testLobBuff = Commlib.getRandomBytes( writeSize );
        long modifyTime = 0;
        try ( DBLob lob = cl.createLob( oid )) {
            lob.write( testLobBuff );
            long createTime = lob.getCreateTime();
            modifyTime = lob.getModificationTime();
            if ( createTime > modifyTime ) {
                Assert.assertTrue( false,
                        "the modify time must not be less than the create time!" );
            }
        }
        return modifyTime;
    }

    private void lockAndWriteLob( ObjectId oid, byte[] writeBuff, int offset ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lock( offset, writeBuff.length );
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( writeBuff );
        }
    }

    private void getLastModifyTimeAndCheckResult( ObjectId oid,
            long lastModifyTime ) {
        long newModifyTime = 0;
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
            newModifyTime = lob.getModificationTime();
        }

        // check the new modify time ,must be update the time
        if ( newModifyTime <= lastModifyTime ) {
            Assert.assertTrue( false,
                    "the new modify time must not be less than the lastmodify time!" );
        }

        // check the modify time from the db
        long modifyTimeFromDB = 0;
        DBCursor listCursor = cl.listLobs();
        while ( listCursor.hasNext() ) {
            BSONObject obj = listCursor.getNext();
            BSONTimestamp modifyTime = ( BSONTimestamp ) obj
                    .get( "ModificationTime" );
            long microtime = modifyTime.getInc();
            long second = modifyTime.getTime();
            // convert to millisecondes
            modifyTimeFromDB = second * 1000 + microtime / 1000;
        }
        listCursor.close();
        Assert.assertEquals( modifyTimeFromDB, newModifyTime,
                " java clent and db get modifytime inconsistency" );
    }

    private void checkModifyTimeAfterClose( ObjectId oid ) {
        DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE );
        long oldModifyTime = lob.getModificationTime();
        lob.write( new byte[ 128 ] );
        lob.close();
        long newModifyTime = lob.getModificationTime();
        if ( oldModifyTime >= newModifyTime ) {
            Assert.fail( "modification time not updated after close lob" );
        }
    }
}
