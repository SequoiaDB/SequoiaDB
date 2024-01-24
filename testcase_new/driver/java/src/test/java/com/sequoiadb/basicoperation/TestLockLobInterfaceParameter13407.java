package com.sequoiadb.basicoperation;

import java.util.Date;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestLockLobInterfaceParameter13407.java test content:lock lob
 * interface parameter checking,test interface:lock()/lockAndSeek() testlink
 * case:seqDB-13407/13408
 * 
 * @author wuyan
 * @Date 2017.11.23
 * @version 1.00
 */
public class TestLockLobInterfaceParameter13407 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider1")
    public Object[][] generateErrorargument() {
        return new Object[][] {
                // the parameter is offset, length
                // the offset over boundary value:-1
                new Object[] { -1, 1 },
                // the length over boundary value:-2
                new Object[] { 1, -2 },
                // the length over boundary value:0
                new Object[] { 1, 0 },
                // the offset + length over boundary value:0
                new Object[] { 9223372036854775806L, 3 },
                new Object[] { 3, 9223372036854775806L }, };
    }

    @DataProvider(name = "pagesizeProvider2")
    public Object[][] generateLegalargument() {
        return new Object[][] {
                // the parameter is offset, length
                // the offset + length over boundary value
                new Object[] { 9223372036854775806L, 2 },
                // the length over boundary value:-2
                new Object[] { 1, -2 },
                // the length over boundary value:0
                new Object[] { 1, 0 },
                // the offset + length over boundary value:0
                new Object[] { 9223372036854775806L, 3 },
                new Object[] { 3, 9223372036854775806L }, };
    }

    private String clName = "writelob13407";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private byte[] testLobBuff = null;
    private ObjectId oid = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );

            int writeSize = 1024 * 2;
            testLobBuff = Commlib.getRandomBytes( writeSize );
            oid = Commlib.createAndWriteLob( cl, testLobBuff );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }
    }

    @Test(dataProvider = "pagesizeProvider1")
    private void testLob( long offset, long writeSize ) {
        // the lockAndSeek() illegal checkout
        testLockAndSeekError( offset, writeSize );
        // the lock() illegal checkout
        testLockError( offset, writeSize );
    }

    @Test
    private void testLockAndSeek() {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            // test the lockAndSeek(),offset + length = maxlong
            long offset = 9223372036854775806L;
            long length = 1;
            lob.lockAndSeek( offset, length );
            byte[] rewriteBuff = Commlib.getRandomBytes( ( int ) length );
            lob.write( rewriteBuff );
            long actWriteLobSize = lob.getSize();
            long expLobSize = offset + length;
            Assert.assertEquals( actWriteLobSize, expLobSize );

            // test the lock(),offset + length = maxlong
            long offset1 = 9223372036854775805L;
            long length1 = 2;
            lob.lock( offset1, length1 );
            lob.seek( offset1, DBLob.SDB_LOB_SEEK_SET );
            byte[] rewriteBuff1 = Commlib.getRandomBytes( ( int ) length1 );
            lob.write( rewriteBuff1 );
            long actWriteLobSize1 = lob.getSize();
            Assert.assertEquals( actWriteLobSize1, expLobSize );
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

    private void testLockAndSeekError( long offset, long writeLobSize ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lockAndSeek( offset, writeLobSize );
            byte[] rewriteBuff = Commlib.getRandomBytes( ( int ) writeLobSize );
            lob.write( rewriteBuff );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            // expected results to retrun to -6,then ignore exceptions
            Assert.assertEquals( -6, e.getErrorCode(),
                    e.getErrorCode() + e.getMessage() );
        }
    }

    private void testLockError( long offset, long writeLobSize ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lock( offset, writeLobSize );
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            byte[] rewriteBuff = Commlib.getRandomBytes( ( int ) writeLobSize );
            lob.write( rewriteBuff );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            // expected results to retrun to -6,then ignore exceptions
            Assert.assertEquals( -6, e.getErrorCode(),
                    e.getErrorCode() + e.getMessage() );
        }
    }

}
