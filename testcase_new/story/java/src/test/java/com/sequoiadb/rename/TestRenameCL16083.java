package com.sequoiadb.rename;

import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
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
 * @FileName:TestRenameCL16083
 * @content 并发修改cl名和删除相同cl
 * @author chensiqin
 * @Date 2018-10-23
 * @version 1.00
 */
public class TestRenameCL16083 extends SdbTestBase {

    private String clName = "cl16083";
    private String newclName = "newcl16083";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection cl1 = cs.createCollection( clName );
        RenameUtil.insertData( cl1, 5 );
    }

    @Test
    public void test16083() throws Exception {
        ThreadExecutor es = new ThreadExecutor();

        RenameCLThread renameCLThread = new RenameCLThread();
        DropCLThread dropCLThread = new DropCLThread();
        es.addWorker( renameCLThread );
        es.addWorker( dropCLThread );
        es.run();

        if ( dropCLThread.getRetCode() != 0 ) {
            Assert.assertEquals( renameCLThread.getRetCode(), 0 );
            RenameUtil.checkRenameCLResult( sdb, SdbTestBase.csName, clName,
                    newclName );
            Assert.assertFalse( cs.isCollectionExist( clName ) );
            if ( dropCLThread.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && dropCLThread.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode()
                    && dropCLThread
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail(
                        "errcode not expected : " + dropCLThread.getRetCode() );
            }
        } else if ( renameCLThread.getRetCode() != 0 ) {
            Assert.assertEquals( dropCLThread.getRetCode(), 0 );
            Assert.assertFalse( cs.isCollectionExist( clName ) );
            Assert.assertFalse( cs.isCollectionExist( newclName ) );
            if ( renameCLThread.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && renameCLThread.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode()
                    && renameCLThread
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail( "errcode not expected : "
                        + renameCLThread.getRetCode() );
            }
        } else {
            Assert.assertEquals( renameCLThread.getRetCode(), 0 );
            Assert.assertEquals( dropCLThread.getRetCode(), 0 );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, SdbTestBase.csName, clName );
            CommLib.clearCL( sdb, SdbTestBase.csName, newclName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCLThread extends ResultStore {

        @ExecuteOrder(step = 1)
        private void getCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace localcs = db
                        .getCollectionSpace( SdbTestBase.csName );
                localcs.renameCollection( clName, newclName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropCLThread extends ResultStore {

        @ExecuteOrder(step = 1)
        private void getCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace localcs = db
                        .getCollectionSpace( SdbTestBase.csName );
                localcs.dropCollection( clName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

}
