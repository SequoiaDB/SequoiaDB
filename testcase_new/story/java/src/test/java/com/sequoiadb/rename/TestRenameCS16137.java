package com.sequoiadb.rename;

import com.sequoiadb.testcommon.CommLib;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:TestRenameCS16137
 * @content 修改cs名和创建索引并发
 * @author chensiqin
 * @Date 2018-10-20
 * @version 1.00
 */
public class TestRenameCS16137 extends SdbTestBase {

    private String csName = "cs16137";
    private String newCSName = "newcs16137";
    private String clName = "cl16137";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private boolean createOneSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );
        cl.createIndex( "index0", "{a0:1}", false, false );
        cl.createIndex( "index1", "{a1:-1}", false, false );
        cl.createIndex( "index2", "{a2:1}", false, false );
    }

    @Test
    public void test16137() {
        RenameCSThread renameCSThread = new RenameCSThread();
        CreateIndexThread createIndexThread = new CreateIndexThread();
        renameCSThread.start();
        createIndexThread.start();

        if ( renameCSThread.isSuccess() && !createIndexThread.isSuccess() ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, 1 );
            int expIndexNum = createOneSuccess ? 4 : 3;
            checkCLIndex( sdb, newCSName, clName, expIndexNum );
            BaseException e = ( BaseException ) createIndexThread
                    .getExceptions().get( 0 );
            if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( renameCSThread.isSuccess()
                && createIndexThread.isSuccess() ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, 1 );
            checkCLIndex( sdb, newCSName, clName, 5 );
        } else {
            Assert.fail( "renameCSThread must success, but failed : "
                    + renameCSThread.getErrorMsg() );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
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

    public void checkCLIndex( Sequoiadb db, String localCSName,
            String localCLName, int expected ) {
        CollectionSpace localCS = db.getCollectionSpace( localCSName );
        DBCollection localCL = localCS.getCollection( localCLName );
        DBCursor cur = localCL.getIndexes();
        int indexnum = 0;
        while ( cur.hasNext() ) {
            cur.getNext();
            indexnum++;
        }
        Assert.assertEquals( indexnum, expected + 1 );
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                db.renameCollectionSpace( csName, newCSName );
            } finally {
                db.close();
            }
        }
    }

    private class CreateIndexThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db.getCollectionSpace( csName );
                DBCollection localcl = localcs.getCollection( clName );
                localcl.createIndex( "index3", "{a3:1}", false, false );
                createOneSuccess = true;
                localcl.createIndex( "index4", "{a4:1}", false, false );
            } finally {
                db.close();
            }
        }
    }

}