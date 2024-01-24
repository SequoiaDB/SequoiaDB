package com.sequoiadb.rename;

import com.sequoiadb.testcommon.CommLib;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:TestRenameCS16134
 * @content 并发修改两个cs名为同一个cs名
 * @author chensiqin
 * @Date 2018-10-22
 * @version 1.00
 */
public class TestRenameCS16134 extends SdbTestBase {

    private String csName1 = "cs16134_1";
    private String csName2 = "cs16134_2";
    private String newCSName = "newcs16134";
    private String tmpCSName = "tmpcs16134";
    private String clName = "cl16134";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }
        cs = sdb.createCollectionSpace( csName1 );
        cs.createCollection( clName );
        cs = sdb.createCollectionSpace( csName2 );
        cs.createCollection( clName );
    }

    @Test
    public void test16134() {
        RenameCSThread renameCS1Thread = new RenameCSThread( csName1,
                newCSName );
        RenameCSThread renameCS2Thread = new RenameCSThread( csName2,
                newCSName );
        renameCS1Thread.start();
        renameCS2Thread.start();

        if ( renameCS1Thread.isSuccess() && !renameCS2Thread.isSuccess() ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName1, newCSName, 1 );
            Assert.assertTrue( sdb.isCollectionSpaceExist( csName2 ) );
            BaseException e = ( BaseException ) renameCS2Thread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() == -148 ) {
                RenameUtil.retryToRenameCS( sdb, csName2, tmpCSName );
            }
            if ( e.getErrorCode() != -33 && e.getErrorCode() != -147
                    && e.getErrorCode() != -148 && e.getErrorCode() != -190 ) {
                Assert.fail( "renameCS2Thread failed : " + e.getMessage() );
            }
        } else if ( !renameCS1Thread.isSuccess()
                && renameCS2Thread.isSuccess() ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName2, newCSName, 1 );
            Assert.assertTrue( sdb.isCollectionSpaceExist( csName1 ) );
            BaseException e = ( BaseException ) renameCS1Thread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() == -148 ) {
                RenameUtil.retryToRenameCS( sdb, csName1, tmpCSName );
            }
            if ( e.getErrorCode() != -33 && e.getErrorCode() != -147
                    && e.getErrorCode() != -148 && e.getErrorCode() != -190 ) {
                Assert.fail( "renameCS1Thread failed : " + e.getMessage() );
            }
        } else if ( !renameCS1Thread.isSuccess()
                && !renameCS2Thread.isSuccess() ) {
            Assert.fail( "renameCS1Thread and renameCS2Thread all failed :"
                    + renameCS1Thread.getErrorMsg()
                    + renameCS2Thread.getErrorMsg() );
        } else {
            Assert.fail( "renameCS1Thread and renameCS2Thread all success :"
                    + renameCS1Thread.getErrorMsg()
                    + renameCS2Thread.getErrorMsg() );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName1 ) ) {
                sdb.dropCollectionSpace( csName1 );
            }
            if ( sdb.isCollectionSpaceExist( csName2 ) ) {
                sdb.dropCollectionSpace( csName2 );
            }
            if ( sdb.isCollectionSpaceExist( newCSName ) ) {
                sdb.dropCollectionSpace( newCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {
        private String oldName;
        private String newName;

        public RenameCSThread( String oldName, String newName ) {
            super();
            this.oldName = oldName;
            this.newName = newName;
        }

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                db.renameCollectionSpace( this.oldName, this.newName );
            } finally {
                db.close();
            }
        }
    }

}
