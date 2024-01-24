package com.sequoiadb.lob.subcl;

import java.util.ArrayList;
import java.util.List;

import org.bson.types.ObjectId;
import org.testng.Assert;
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
 * @Description seqDB-19073 :: 版本: 1 :: 主表并发创建相同lob
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19073 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String mainCLName = "mainCL_19073";
    private String subCLName = "subCL_19073";
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        mainCL = LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
    }

    @Test
    public void test() throws Exception {
        ObjectId lobId = mainCL.createLobID();
        ThreadExecutor thread = new ThreadExecutor();
        PutLobThread putLob1 = new PutLobThread( lobId );
        PutLobThread putLob2 = new PutLobThread( lobId );
        thread.addWorker( putLob1 );
        thread.addWorker( putLob2 );
        thread.run();

        if ( putLob1.getRetCode() == putLob2.getRetCode() ) {
            Assert.fail(
                    "threads that insert the same lob concurrently have the same results, thread1: "
                            + putLob1.getRetCode() + ", thread2: "
                            + putLob2.getRetCode() );
        }
        List< ObjectId > lobIds = new ArrayList< ObjectId >();
        lobIds.add( lobId );
        LobSubUtils.checkLobMD5( mainCL, lobIds, lobBuff );
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

    private class PutLobThread extends ResultStore {

        private ObjectId id;

        public PutLobThread( ObjectId lobId ) {
            this.id = lobId;
        }

        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                DBLob lob = mainCL.createLob( id );
                lob.write( lobBuff );
                lob.close();
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
                if ( e.getErrorCode() != -5 && e.getErrorCode() != -297
                        && e.getErrorCode() != -317 ) {
                    throw e;
                }
            }
        }
    }

}
