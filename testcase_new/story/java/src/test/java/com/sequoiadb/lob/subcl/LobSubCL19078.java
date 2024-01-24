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
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19078 :: 版本: 1 :: 主表并发读和truncateLob
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19078 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String mainCLName = "mainCL_19078";
    private String subCLName = "subCL_19078";
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024 * 50;
    private byte[] lobBuff;
    private ObjectId lobId;
    private String expMD5;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        mainCL = LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        expMD5 = LobOprUtils.getMd5( lobBuff );
        DBLob lob = mainCL.createLob();
        lob.write( lobBuff );
        lobId = lob.getID();
        lob.close();
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor();
        TruncateLobThread truncate = new TruncateLobThread();
        thread.addWorker( new ReadLobThread() );
        thread.addWorker( truncate );
        thread.run();

        if ( truncate.getRetCode() == 0 ) {
            byte[] expBuff = new byte[ writeLobSize / 2 ];
            System.arraycopy( lobBuff, 0, expBuff, 0, writeLobSize / 2 );
            List< ObjectId > lobIds = new ArrayList< ObjectId >();
            lobIds.add( lobId );
            LobSubUtils.checkLobMD5( mainCL, lobIds, expBuff );
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

    private class ReadLobThread {

        @ExecuteOrder(step = 1)
        private void readLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                byte[] data = new byte[ lobBuff.length ];
                DBLob lob = mainCL.openLob( lobId );
                lob.read( data );
                lob.close();
                String actMD5 = LobOprUtils.getMd5( lobBuff );
                if ( !actMD5.equals( expMD5 ) ) {
                    throw new BaseException( 0,
                            "check lob: " + lobId + " md5 error, exp: " + expMD5
                                    + ", act: " + actMD5 );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -317 ) {
                    throw e;
                }
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
