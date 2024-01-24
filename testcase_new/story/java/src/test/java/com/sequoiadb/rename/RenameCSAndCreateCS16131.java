package com.sequoiadb.rename;

import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName RenameCSAndCreateCS16131.java
 * @content concurrent rename cs and create cs, the create cs name is the same
 *          as the new cs name
 * @testlink seqDB-16131
 * @author wuyan
 * @Date 2018.10.29
 * @version 1.00
 */
public class RenameCSAndCreateCS16131 extends SdbTestBase {

    private String csName = "renameCS161311";
    private String newCSName = "renameCSNew161311";
    private String clName = "rename16131";
    private Sequoiadb sdb = null;
    private int pageSizeByCreateCS = 32768;
    private int pageSizeByRenameCS = 65536;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        RenameUtil.removeCS( sdb, newCSName );
        String option = "{ PageSize : " + pageSizeByRenameCS + "}";
        RenameUtil.createCS( sdb, csName, option );
    }

    @Test
    public void test() {
        CreateCSThread createCSThread = new CreateCSThread();
        RenameCSThread renameCSThread = new RenameCSThread();
        createCSThread.start();
        renameCSThread.start();

        if ( renameCSThread.isSuccess() ) {
            Assert.assertTrue( !createCSThread.isSuccess(),
                    createCSThread.getErrorMsg() );
            BaseException e = ( BaseException ) ( createCSThread.getExceptions()
                    .get( 0 ) );
            Assert.assertEquals( -33, e.getErrorCode(),
                    "createCS fail:" + createCSThread.getErrorMsg() );
            // test the newCS is available:create cl
            createCL( newCSName, clName );
            int clNum = 1;
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, clNum );
            testCSInfo( newCSName, pageSizeByRenameCS );
        } else {
            Assert.assertTrue( createCSThread.isSuccess(),
                    createCSThread.getErrorMsg() );
            BaseException e = ( BaseException ) ( renameCSThread.getExceptions()
                    .get( 0 ) );
            Assert.assertEquals( -33, e.getErrorCode(),
                    "renameCS fail:" + renameCSThread.getErrorMsg() );
            testCSInfo( newCSName, pageSizeByCreateCS );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RenameUtil.removeCS( sdb, newCSName );
            RenameUtil.removeCS( sdb, csName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    public class RenameCSThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csName, newCSName );
            }
        }
    }

    public class CreateCSThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject option = new BasicBSONObject();
                option.put( "PageSize", pageSizeByCreateCS );
                db.createCollectionSpace( newCSName, option );
            }
        }
    }

    private void createCL( String csName, String clName ) {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        cs.createCollection( clName );
    }

    // verify the cs is createCS or renameCS by pageSize
    private void testCSInfo( String csName, long expPageSize ) {
        try ( DBCursor cursor = sdb.getSnapshot(
                Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                "{'Name':'" + csName + "'}", "", "" )) {
            // detach cl fail, than the subcl is exist on the maincl
            while ( cursor.hasNext() ) {
                BSONObject objInfo = cursor.getNext();
                int pageSize = ( int ) objInfo.get( "PageSize" );
                Assert.assertEquals( pageSize, expPageSize );
            }
        }
    }
}
