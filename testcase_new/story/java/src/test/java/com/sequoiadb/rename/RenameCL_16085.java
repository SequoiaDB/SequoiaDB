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
 * @Description RenameCL_16085.java 并发修改cl1名和创建cl2，其中创建cl名为更新名
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_16085 extends SdbTestBase {

    private String clName = "rename_CL_16085";
    private String newCLName = "rename_CL_16085_new";
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
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCLThread renameCLThread = new RenameCLThread();
        CreateCLBThread createCLThread = new CreateCLBThread();

        renameCLThread.start();
        createCLThread.start();

        boolean renameCL = renameCLThread.isSuccess();
        boolean createCL = createCLThread.isSuccess();

        if ( !renameCL ) {
            Integer[] errnosA = { -22, -147 };
            BaseException errorA = ( BaseException ) renameCLThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( renameCLThread.getErrorMsg() );
            }
        }

        if ( !createCL ) {
            Integer[] errnosB = { -22, -147 };
            BaseException errorB = ( BaseException ) createCLThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( createCLThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( renameCL && !createCL ) {
                RenameUtil.checkRenameCLResult( db, SdbTestBase.csName, clName,
                        newCLName );
            } else if ( !renameCL && createCL ) {
                cs = db.getCollectionSpace( SdbTestBase.csName );

                Assert.assertTrue( cs.isCollectionExist( clName ) );
                long actNum1 = cs.getCollection( clName ).getCount();
                Assert.assertEquals( actNum1, recordNum );

                Assert.assertTrue( cs.isCollectionExist( newCLName ) );
                long actNum2 = cs.getCollection( newCLName ).getCount();
                Assert.assertEquals( actNum2, 0 );
            } else if ( !renameCL && !createCL ) {
                Assert.fail( "rename cl failed: " + renameCLThread.getErrorMsg()
                        + ", create cl failed: "
                        + createCLThread.getErrorMsg() );
            } else {
                Assert.fail(
                        "rename cl and create cl to the same name, all success" );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, SdbTestBase.csName, clName );
            CommLib.clearCL( sdb, SdbTestBase.csName, newCLName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.renameCollection( clName, newCLName );
            } finally {
                db.close();
            }
        }
    }

    private class CreateCLBThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.createCollection( newCLName );
            }
        }
    }

}
