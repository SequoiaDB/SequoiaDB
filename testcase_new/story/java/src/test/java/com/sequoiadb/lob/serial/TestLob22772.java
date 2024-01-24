package com.sequoiadb.lob.serial;

import org.bson.BasicBSONObject;
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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-22772:多线程写lob过程中执行sync
 * @Author luweikang
 * @Date 2021.1.7
 * @Version 1.00
 */

public class TestLob22772 extends SdbTestBase {

    private String csName = "cs22772";
    private String clName = "cl22772";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private ObjectId oid = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName,
                new BasicBSONObject( "ReplSize", 0 ) );
        DBLob lob = cl.createLob();
        oid = lob.getID();
        lob.close();
    }

    @Test()
    public void test() {
        WriteLobThread write = new WriteLobThread();
        SyncThread sync = new SyncThread();

        write.start( 10 );
        sync.start();

        Assert.assertTrue( write.isSuccess(), write.getErrorMsg() );
        Assert.assertTrue( sync.isSuccess(), sync.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private class WriteLobThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < 1000; i++ ) {
                    try {
                        DBCollection cl = db.getCollectionSpace( csName )
                                .getCollection( clName );
                        DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE );
                        byte[] src = { 'a', 'b', 'c' };
                        lob.write( src, 0, 3 );
                        lob.seek( 0, DBLob.SDB_LOB_SEEK_SET );
                        byte[] src2 = { '1', '2', '3', '4' };
                        lob.write( src2 );
                        lob.close();
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_LOB_LOCK_CONFLICTED
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                }
            }
        }
    }

    private class SyncThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < 1000; i++ ) {
                    db.sync( new BasicBSONObject( "Block", true )
                            .append( "CollectionSpace", csName ) );
                    Thread.sleep( 10 );
                }
            }
        }

    }

}
