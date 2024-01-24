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
 * @FileName:TestRenameCL16082
 * @content 并发修改cl，其中一个cl修改为另一个cl旧名
 * @author chensiqin
 * @Date 2018-10-23
 * @version 1.00
 */
public class TestRenameCL16082 extends SdbTestBase {

    private String clName1 = "cl16082_1";
    private String clName2 = "cl16082_2";
    private String newclName = "newcl16082";
    private String tmpCLName = "tmpcl16082";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private int insertNum1 = 5;// cl1中插入的记录数
    private int insertNum2 = 20;// cl2中插入的记录数

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection cl1 = cs.createCollection( clName1 );
        DBCollection cl2 = cs.createCollection( clName2 );
        RenameUtil.insertData( cl1, insertNum1 );
        RenameUtil.insertData( cl2, insertNum2 );
    }

    @Test
    public void test16082() {
        RenameCLThread rename1 = new RenameCLThread( clName1, newclName );
        RenameCLThread rename2 = new RenameCLThread( clName2, clName1 );
        rename1.start();
        rename2.start();
        if ( rename1.isSuccess() && rename2.isSuccess() ) {
            checkCLRecord( newclName, insertNum1 );
            checkCLRecord( clName1, insertNum2 );
            Assert.assertTrue( cs.isCollectionExist( newclName ) );
            Assert.assertTrue( cs.isCollectionExist( clName1 ) );
            Assert.assertFalse( cs.isCollectionExist( clName2 ) );
        } else if ( rename1.isSuccess() && !rename2.isSuccess() ) {
            RenameUtil.checkRenameCLResult( sdb, SdbTestBase.csName, clName1,
                    newclName );
            checkCLRecord( newclName, insertNum1 );
            checkCLRecord( clName2, insertNum2 );
            BaseException e = ( BaseException ) rename2.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() == -148 ) {
                RenameUtil.retryToRenameCL( sdb, csName, clName2, tmpCLName );
            }
            if ( e.getErrorCode() != -22 && e.getErrorCode() != -147
                    && e.getErrorCode() != -148 && e.getErrorCode() != -190 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else {
            Assert.fail( "test16082 failed: " + rename1.getErrorMsg()
                    + rename2.getErrorMsg() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName1 ) ) {
                cs.dropCollection( clName1 );
            }
            if ( cs.isCollectionExist( clName2 ) ) {
                cs.dropCollection( clName2 );
            }
            if ( cs.isCollectionExist( newclName ) ) {
                cs.dropCollection( newclName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }

    }

    public void checkCLRecord( String name, int expected ) {
        DBCollection cl = cs.getCollection( name );
        Assert.assertEquals( cl.getCount(), expected );
    }

    private class RenameCLThread extends SdbThreadBase {
        private String oldName;
        private String newName;

        public RenameCLThread( String oldName, String newName ) {
            super();
            this.oldName = oldName;
            this.newName = newName;
        }

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db.getCollectionSpace( csName );
                localcs.renameCollection( this.oldName, this.newName );

            } finally {
                db.close();
            }
        }
    }

}
