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
 * @Description RenameCL_16081.java 并发修改cl为相同名
 * @author luweikang
 * @date 2018年10月17日
 * @review wuyan 2018.10.31
 */
public class RenameCL_16081 extends SdbTestBase {

    private String clNameA = "rename_CL_16081A";
    private String clNameB = "rename_CL_16081B";
    private String newCLName = "rename_CL_16081_new";
    private String tmpCLName = "tmpCLName_16081";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection clA = null;
    private DBCollection clB = null;
    private int recordNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        clA = cs.createCollection( clNameA );
        clB = cs.createCollection( clNameB );
        RenameUtil.insertData( clA, recordNum );
        RenameUtil.insertData( clB, recordNum );
    }

    @Test
    public void test() {
        RenameCLAThread reCLANameThread = new RenameCLAThread();
        RenameCLBThread reCLBNameThread = new RenameCLBThread();

        reCLANameThread.start();
        reCLBNameThread.start();

        boolean renameCLA = reCLANameThread.isSuccess();
        boolean renameCLB = reCLBNameThread.isSuccess();

        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !renameCLA ) {
            Integer[] errnosA = { -22, -148 };
            BaseException errorA = ( BaseException ) reCLANameThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( reCLANameThread.getErrorMsg() );
            }
            if ( errorA.getErrorCode() == -148 ) {
                RenameUtil.retryToRenameCL( db, csName, clNameA, tmpCLName );
            }
        }

        if ( !renameCLB ) {
            Integer[] errnosB = { -22, -148 };
            BaseException errorB = ( BaseException ) reCLBNameThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( reCLBNameThread.getErrorMsg() );
            }
            if ( errorB.getErrorCode() == -148 ) {
                RenameUtil.retryToRenameCL( db, csName, clNameB, tmpCLName );
            }
        }

        // java驱动会有缓存,需要从新获取连接,变量名缩写已修改
        try {
            if ( renameCLA && !renameCLB ) {
                RenameUtil.checkRenameCLResult( db, SdbTestBase.csName, clNameA,
                        newCLName );
                Assert.assertTrue(
                        db.getCollectionSpace( csName )
                                .isCollectionExist( clNameB ),
                        "clB rename faild, should exist : " + clNameB );
            } else if ( !renameCLA && renameCLB ) {
                RenameUtil.checkRenameCLResult( db, SdbTestBase.csName, clNameB,
                        newCLName );
                Assert.assertTrue(
                        db.getCollectionSpace( csName )
                                .isCollectionExist( clNameA ),
                        "clB rename faild, should exist : " + clNameA );
            } else if ( !renameCLA && !renameCLB ) {
                Assert.fail( "rename cl name to the same name, all failed" );
            } else {
                Assert.fail( "rename cl name to the same name, all success" );
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
            CommLib.clearCL( sdb, SdbTestBase.csName, clNameA );
            CommLib.clearCL( sdb, SdbTestBase.csName, clNameB );
            CommLib.clearCL( sdb, SdbTestBase.csName, newCLName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLAThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.renameCollection( clNameA, newCLName );
            }
        }
    }

    private class RenameCLBThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.renameCollection( clNameB, newCLName );
            }
        }
    }

}
