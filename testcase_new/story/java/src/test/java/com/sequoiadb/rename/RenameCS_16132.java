package com.sequoiadb.rename;

import java.util.Arrays;

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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCS_16132.java 修改cs名和修改cl名并发
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCS_16132 extends SdbTestBase {

    private String csName = "renameCS_16132";
    private String newCSName = "renameCS_16132_new";
    private String clName = "renameCS_CL_16132";
    private String newCLName = "renameCS_CL_16132_new";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private int recordNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCSThread reCSNameThread = new RenameCSThread();
        RenameCLThread reCLNameThread = new RenameCLThread();

        reCSNameThread.start();
        reCLNameThread.start();

        boolean csRename = reCSNameThread.isSuccess();
        boolean clRename = reCLNameThread.isSuccess();

        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !csRename ) {
            Integer[] errnosA = { -22, -148, -147 };
            BaseException errorA = ( BaseException ) reCSNameThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( reCSNameThread.getErrorMsg() );
            }
            if ( errorA.getErrorCode() == -148 && clRename ) {
                RenameUtil.retryToRenameCS( db, csName, newCSName );
            }
        }

        if ( !clRename ) {
            Integer[] errnosB = { -23, -34, -148, -147 };
            BaseException errorB = ( BaseException ) reCLNameThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( reCLNameThread.getErrorMsg() );
            }
            if ( errorB.getErrorCode() == -148 && clRename ) {
                RenameUtil.retryToRenameCL( db, newCSName, clName, newCLName );
            }
        }

        try {
            if ( csRename && clRename ) {
                RenameUtil.checkRenameCSResult( db, csName, newCSName, 1 );
                RenameUtil.checkRenameCLResult( db, newCSName, clName,
                        newCLName );
            } else if ( !csRename && clRename ) {
                RenameUtil.checkRenameCSResult( db, newCSName, csName, 1 );
                RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
            } else if ( csRename && !clRename ) {
                RenameUtil.checkRenameCSResult( db, csName, newCSName, 1 );
                RenameUtil.checkRenameCLResult( db, newCSName, newCLName,
                        clName );
            } else if ( !csRename && !clRename ) {
                Assert.fail( "Concurrent to renameCS and renameCL failed" );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, csName );
            CommLib.clearCS( sdb, newCSName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csName, newCSName );
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace sdbcs = db.getCollectionSpace( csName );
                Thread.sleep( 2000 );
                sdbcs.renameCollection( clName, newCLName );
            }
        }
    }

}
