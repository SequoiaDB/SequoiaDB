package com.sequoiadb.lob.subcl;

import java.util.ArrayList;
import java.util.List;

import org.bson.types.ObjectId;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19077 :: 版本: 1 :: 主表并发写和truncateLob
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19077 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String mainCLName = "mainCL_19077";
    private String subCLName = "subCL_19077";
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024 * 50;
    private byte[] lobBuff;
    private ObjectId lobId;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        mainCL = LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        lobId = mainCL.createLobID();
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor();
        TruncateLobThread truncateLob = new TruncateLobThread();
        thread.addWorker( new PutLobThread() );
        thread.addWorker( truncateLob );
        thread.run();

        List< ObjectId > idList = new ArrayList< ObjectId >();
        idList.add( lobId );
        if ( truncateLob.getRetCode() == 0 ) {
            byte[] expBuff = new byte[ writeLobSize / 2 ];
            System.arraycopy( lobBuff, 0, expBuff, 0, writeLobSize / 2 );
            LobSubUtils.checkLobMD5( mainCL, idList, expBuff );
        } else {
            LobSubUtils.checkLobMD5( mainCL, idList, lobBuff );
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

    private class PutLobThread {

        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                DBLob lob = mainCL.createLob( lobId );
                lob.write( lobBuff );
                lob.close();
            }
        }
    }

    private class TruncateLobThread extends ResultStore {

        @ExecuteOrder(step = 1)
        private void truncateLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                try {
                    Thread.sleep( 2000 );
                } catch ( InterruptedException e ) {
                }
                mainCL.truncateLob( lobId, writeLobSize / 2 );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
                if ( e.getErrorCode() != -317 ) {
                    throw e;
                }
            }
        }
    }

}
