package com.sequoiadb.rename;

import com.sequoiadb.testcommon.CommLib;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:TestRenameCL16095
 * @content 并发修改cl名和删除cs
 * @author chensiqin
 * @Date 2018-10-23
 * @version 1.00
 */
public class TestRenameCL16095 extends SdbTestBase {

    private String clName = "cl16095";
    private String newclName = "newcl16095";
    private String localCSName = "cs16095";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( localCSName ) ) {
            sdb.dropCollectionSpace( localCSName );
        }
        cs = sdb.createCollectionSpace( localCSName );
        DBCollection cl1 = cs.createCollection( clName );
        RenameUtil.insertData( cl1, 5 );
    }

    @Test
    public void test16095() {
        RenameCLThread renameCLThread = new RenameCLThread();
        DropCSThread dropCSThread = new DropCSThread();
        dropCSThread.start();
        renameCLThread.start();

        if ( renameCLThread.isSuccess() && !dropCSThread.isSuccess() ) {
            RenameUtil.checkRenameCLResult( sdb, localCSName, clName,
                    newclName );
            BaseException e = ( BaseException ) dropCSThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( !renameCLThread.isSuccess() && dropCSThread.isSuccess() ) {
            Assert.assertEquals( sdb.isCollectionSpaceExist( localCSName ),
                    false );
            BaseException e = ( BaseException ) renameCLThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -23 && e.getErrorCode() != -147
                    && e.getErrorCode() != -34 && e.getErrorCode() != -190 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( !renameCLThread.isSuccess() && !dropCSThread.isSuccess() ) {
            Assert.fail( "test16095 failed :" + renameCLThread.getErrorMsg()
                    + dropCSThread.getErrorMsg() );
        } else {
            Assert.assertEquals( sdb.isCollectionSpaceExist( localCSName ),
                    false );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( localCSName ) ) {
                sdb.dropCollectionSpace( localCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db.getCollectionSpace( localCSName );
                localcs.renameCollection( clName, newclName );

            } finally {
                db.close();
            }
        }
    }

    private class DropCSThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                db.dropCollectionSpace( localCSName );
            } finally {
                db.close();
            }
        }
    }
}
