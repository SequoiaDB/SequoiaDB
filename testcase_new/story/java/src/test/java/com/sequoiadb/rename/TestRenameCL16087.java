package com.sequoiadb.rename;

import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestRenameCL16087
 * @content 并发修改子表名和attach子表
 * @author chensiqin
 * @Date 2018-10-24
 * @version 1.00
 */
public class TestRenameCL16087 extends SdbTestBase {

    private String subCLName = "subCL16087";
    private String newSubCLName = "newSubCL16087";
    private String mainCLName = "mainCL16087";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection mainCL = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        createMainCL();
        cs.createCollection( subCLName, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ) );
    }

    @Test
    public void test16087() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        AttachCLThread attachCLThread = new AttachCLThread();
        RenameSubCLThread renameSubCL = new RenameSubCLThread();
        es.addWorker( attachCLThread );
        es.addWorker( renameSubCL );
        es.run();

        if ( attachCLThread.getRetCode() == 0
                && renameSubCL.getRetCode() == 0 ) {
            Assert.assertFalse( cs.isCollectionExist( subCLName ) );
            mainCL.detachCollection( SdbTestBase.csName + "." + newSubCLName );
        } else if ( attachCLThread.getRetCode() == 0
                && renameSubCL.getRetCode() != 0 ) {
            mainCL.detachCollection( SdbTestBase.csName + "." + newSubCLName );
            if ( renameSubCL.getRetCode() != SDBError.SDB_LOCK_FAILED
                    .getErrorCode()
                    && renameSubCL
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail(
                        "errcode not expected : " + renameSubCL.getRetCode() );
            }
        } else if ( attachCLThread.getRetCode() != 0
                && renameSubCL.getRetCode() == 0 ) {
            try {
                DBCollection maincl = cs.getCollection( mainCLName );
                maincl.detachCollection(
                        SdbTestBase.csName + "." + newSubCLName );
                Assert.fail(
                        "cl attachCollection ok, expected attachCollection fail!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_INVALID_SUB_CL
                        .getErrorCode() ) {
                    throw e;
                }
            }
            if ( attachCLThread.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && attachCLThread.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode()
                    && attachCLThread
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail( "errcode not expected : "
                        + attachCLThread.getRetCode() );
            }
        } else {
            Assert.fail( "attachCLThread : " + attachCLThread.getRetCode()
                    + "\n subCLThread : " + renameSubCL.getRetCode() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
            if ( cs.isCollectionExist( newSubCLName ) ) {
                cs.dropCollection( newSubCLName );
            }
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void createMainCL() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        options.put( "ShardingKey", opt );
        options.put( "ShardingType", "range" );
        mainCL = cs.createCollection( mainCLName, options );
    }

    private class RenameSubCLThread extends ResultStore {
        @ExecuteOrder(step = 1)
        private void renameSubCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace localcs = db
                        .getCollectionSpace( SdbTestBase.csName );
                localcs.renameCollection( subCLName, newSubCLName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class AttachCLThread extends ResultStore {
        @ExecuteOrder(step = 1)
        private void attachCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = cs.getCollection( mainCLName );
                BSONObject options = new BasicBSONObject();
                options.put( "LowBound", new BasicBSONObject( "a", 1 ) );
                options.put( "UpBound", new BasicBSONObject( "a", 100 ) );
                maincl.attachCollection( SdbTestBase.csName + "." + subCLName,
                        options );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
