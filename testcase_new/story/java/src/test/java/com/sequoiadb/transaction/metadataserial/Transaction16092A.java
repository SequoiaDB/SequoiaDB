package com.sequoiadb.transaction.metadataserial;

import java.util.Arrays;
import java.util.Random;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.rename.RenameUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description RenameCL_16092.java 并发事务操作和修改cl名
 * @author luweikang
 * @date 2018年10月17日
 */
public class Transaction16092A extends SdbTestBase {

    private String csName = "renameCS_16092A";
    private String clName = "rename_CL_16092A";
    private String newCLName = "rename_CL_16092A_new";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private int recordNum = 2000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() {
        RenameCLThread renameCLThread = new RenameCLThread();
        TransactionThread transThread = new TransactionThread();

        renameCLThread.start();
        transThread.start();

        boolean rename = renameCLThread.isSuccess();
        boolean trans = transThread.isSuccess();

        if ( !rename ) {
            Integer[] errnosA = { -190 };
            BaseException errorA = ( BaseException ) renameCLThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( renameCLThread.getErrorMsg() );
            }
        }

        if ( !trans ) {
            Integer[] errnosB = { -23, -190 };
            BaseException errorB = ( BaseException ) transThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( transThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( rename && !trans ) {
                RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
                checkRecordNum( db, newCLName, 0 );
            } else if ( !rename && trans ) {
                cs = db.getCollectionSpace( csName );
                if ( cs.isCollectionExist( newCLName ) ) {
                    Assert.fail( "cl is been rename faild, should not exist" );
                }
                checkRecordNum( db, clName, recordNum );
            } else if ( rename && trans ) {
                RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
                checkRecordNum( db, newCLName, recordNum );
            } else {
                Assert.fail( "rename cl and trans all failed" );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                Thread.sleep( new Random().nextInt( 50 ) + 50 );
                cs.renameCollection( clName, newCLName );
            }
        }
    }

    private class TransactionThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                TransUtils.beginTransaction( db );
                RenameUtil.insertData( cl, recordNum );
                db.commit();
            }
        }
    }

    private void checkRecordNum( Sequoiadb db, String clName,
            int expRecoreNum ) {

        DBCollection checkCL = db.getCollectionSpace( csName )
                .getCollection( clName );
        long actRecordNum = checkCL.getCount();
        Assert.assertEquals( actRecordNum, expRecoreNum, "check cl recordNum" );
    }

}
