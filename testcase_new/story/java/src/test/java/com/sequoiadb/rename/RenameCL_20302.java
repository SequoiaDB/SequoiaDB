package com.sequoiadb.rename;

import java.util.Arrays;
import java.util.Random;

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
 * @Description RenameCS_20302.java 并发修改cl1、cl2名，cl1和cl2相同
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_20302 extends SdbTestBase {

    private String clName = "renameCL_20302";
    private String newCLName = "renameCL_20302_new";
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
        RenameCLAThread reCLANameThread = new RenameCLAThread();
        RenameCLBThread reCLBNameThread = new RenameCLBThread();

        reCLANameThread.start();
        reCLBNameThread.start();

        boolean clARename = reCLANameThread.isSuccess();
        boolean clBRename = reCLBNameThread.isSuccess();

        Assert.assertTrue( clARename, reCLANameThread.getErrorMsg() );

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( !clBRename ) {
                Integer[] errnos = { -23, -147, -148, -190 };
                BaseException error = ( BaseException ) reCLBNameThread
                        .getExceptions().get( 0 );
                System.out.println( error.getErrorCode() );
                if ( !Arrays.asList( errnos )
                        .contains( error.getErrorCode() ) ) {
                    Assert.fail( reCLBNameThread.getErrorMsg() );
                }
                RenameUtil.checkRenameCLResult( db, SdbTestBase.csName, clName,
                        newCLName );
            } else {
                RenameUtil.checkRenameCLResult( db, SdbTestBase.csName,
                        newCLName, clName );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, csName, clName );
            CommLib.clearCL( sdb, csName, newCLName );
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
                db.getCollectionSpace( SdbTestBase.csName )
                        .renameCollection( clName, newCLName );
            }
        }
    }

    private class RenameCLBThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Thread.sleep( new Random().nextInt( 10 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( SdbTestBase.csName )
                        .renameCollection( newCLName, clName );
            }
        }
    }

}
