package com.sequoiadb.lob.subcl;

import java.util.List;
import java.util.Random;

import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19071 :: 版本: 1 :: 并发主表去挂载子表和子表增删改lob
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19071B extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String mainCLName = "mainCL_19071B";
    private String subCLName = "subCL_19071B";
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;
    private List< ObjectId > putOids;
    private List< ObjectId > readOids;
    private List< ObjectId > deleteOids;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        mainCL = LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        readOids = LobSubUtils.createAndWriteLob( mainCL, lobBuff );
        deleteOids = LobSubUtils.createAndWriteLob( mainCL, lobBuff );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor();
        thread.addWorker( new DetachCLThread() );
        thread.addWorker( new PutLobThread() );
        thread.addWorker( new ReadLobThread() );
        thread.addWorker( new DeleteLobThread() );
        thread.run();

        Assert.assertFalse( isAttachMainCL( sdb, csName, subCLName ),
                "check sub cl attach when it is detach" );
        DBCollection subCL = sdb.getCollectionSpace( csName )
                .getCollection( subCLName );
        Assert.assertTrue(
                LobSubUtils.checkLobMD5( subCL, readOids, lobBuff ) );
        Assert.assertTrue( LobSubUtils.checkLobMD5( subCL, putOids, lobBuff ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DetachCLThread {

        @ExecuteOrder(step = 1)
        private void detachCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                try {
                    Thread.sleep( new Random().nextInt( 1000 ) );
                } catch ( InterruptedException e ) {
                }
                maincl.detachCollection( csName + "." + subCLName );
            }
        }
    }

    private class PutLobThread {
        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName );
                putOids = LobSubUtils.createAndWriteLob( subcl, lobBuff );
            }
        }
    }

    private class ReadLobThread {
        @ExecuteOrder(step = 1)
        private void getLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName );
                LobSubUtils.checkLobMD5( subcl, readOids, lobBuff );
            }
        }
    }

    private class DeleteLobThread {
        @ExecuteOrder(step = 1)
        private void deleteLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName );
                for ( ObjectId oid : deleteOids ) {
                    subcl.removeLob( oid );
                }
            }
        }
    }

    private boolean isAttachMainCL( Sequoiadb db, String csName,
            String subCLName ) {
        String clFullName = csName + "." + subCLName;
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", clFullName ), null, null );
        if ( cursor.hasNext() ) {
            BasicBSONObject subCLInfo = ( BasicBSONObject ) cursor.getNext();
            return subCLInfo.containsField( "MainCLName" );
        }
        cursor.close();
        return false;
    }

}
