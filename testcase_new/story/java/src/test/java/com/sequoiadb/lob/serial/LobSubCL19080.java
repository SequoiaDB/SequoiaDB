package com.sequoiadb.lob.serial;

import java.util.List;

import org.bson.types.ObjectId;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19080 :: 版本: 1 :: 并发rename主表集合名和读写删lob
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19080 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String oldMainCLName = "mainCL_19080old";
    private String newMainCLName = "mainCL_19080new";
    private String subCLName = "subCL_19080";
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;
    private List< ObjectId > lobIds1;
    private List< ObjectId > lobIds2;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        mainCL = LobSubUtils.createMainCLAndAttachCL( sdb, csName,
                oldMainCLName, subCLName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        lobIds1 = LobSubUtils.createAndWriteLob( mainCL, lobBuff );
        lobIds2 = LobSubUtils.createAndWriteLob( mainCL, lobBuff );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor( 600000 );
        thread.addWorker( new ReadLobThread() );
        thread.addWorker( new RemoveLobThread() );
        thread.addWorker( new PutLobThread() );
        thread.addWorker( new RenameThread() );
        thread.run();

        mainCL = sdb.getCollectionSpace( csName )
                .getCollection( newMainCLName );
        LobSubUtils.checkLobMD5( mainCL, lobIds1, lobBuff );
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( oldMainCLName ) ) {
                cs.dropCollection( oldMainCLName );
            }
            if ( cs.isCollectionExist( newMainCLName ) ) {
                cs.dropCollection( newMainCLName );
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

    private class ReadLobThread {

        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( oldMainCLName );
                LobSubUtils.checkLobMD5( mainCL, lobIds1, lobBuff );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

    private class RemoveLobThread {

        @ExecuteOrder(step = 1)
        private void removeLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( oldMainCLName );
                for ( ObjectId lobId : lobIds2 ) {
                    mainCL.removeLob( lobId );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

    private class PutLobThread {

        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( oldMainCLName );
                LobSubUtils.createAndWriteLob( mainCL, lobBuff );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

    private class RenameThread {

        @ExecuteOrder(step = 1)
        private void renameCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                }
                db.getCollectionSpace( csName ).renameCollection( oldMainCLName,
                        newMainCLName );
            }
        }
    }

}
