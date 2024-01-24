package com.sequoiadb.lob.subcl;

import java.util.List;
import java.util.Random;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19072 :: 版本: 1 :: 并发主表增删改lob和删除子表
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19072 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String mainCLName = "mainCL_19072";
    private String subCLName = "subCL_19072";
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;
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
        thread.addWorker( new DropCLThread() );
        thread.addWorker( new PutLobThread() );
        thread.addWorker( new ReadLobThread() );
        thread.addWorker( new DeleteLobThread() );
        thread.run();

        DBCursor cur = mainCL.listLobs();
        if ( cur.hasNext() ) {
            Assert.fail( "The lob should not exist when subcl it is drop: "
                    + cur.getNext().toString() );
        }
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

    private class DropCLThread {

        @ExecuteOrder(step = 1)
        private void dropCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                try {
                    Thread.sleep( new Random().nextInt( 1000 ) );
                } catch ( InterruptedException e ) {
                }
                db.getCollectionSpace( csName ).dropCollection( subCLName );
            }
        }
    }

    private class PutLobThread {
        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                LobSubUtils.createAndWriteLob( maincl, lobBuff );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -135 ) {
                    throw e;
                }
            }
        }
    }

    private class ReadLobThread {
        @ExecuteOrder(step = 1)
        private void getLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                LobSubUtils.checkLobMD5( maincl, readOids, lobBuff );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -135 ) {
                    throw e;
                }
            }
        }
    }

    private class DeleteLobThread {
        @ExecuteOrder(step = 1)
        private void deleteLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                for ( ObjectId oid : deleteOids ) {
                    maincl.removeLob( oid );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -135 ) {
                    throw e;
                }
            }
        }
    }

}
