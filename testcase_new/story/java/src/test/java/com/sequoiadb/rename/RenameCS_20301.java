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
 * @Description RenameCS_20301.java 并发修改cs1、cs2名，cs1和cs2相同
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCS_20301 extends SdbTestBase {

    private String csName = "renameCS_20301";
    private String newCSName = "renameCS_20301_new";
    private String clName = "renameCS_CL_20301";
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
        RenameCSAThread reCSANameThread = new RenameCSAThread();
        RenameCSBThread reCSBNameThread = new RenameCSBThread();

        reCSANameThread.start();
        reCSBNameThread.start();

        boolean csARename = reCSANameThread.isSuccess();
        boolean csBRename = reCSBNameThread.isSuccess();

        Assert.assertTrue( csARename, reCSANameThread.getErrorMsg() );

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( !csBRename ) {
                Integer[] errnos = { -34, -147, -148, -190 };
                BaseException error = ( BaseException ) reCSBNameThread
                        .getExceptions().get( 0 );
                if ( !Arrays.asList( errnos )
                        .contains( error.getErrorCode() ) ) {
                    Assert.fail( reCSBNameThread.getErrorMsg() );
                }
                RenameUtil.checkRenameCSResult( db, csName, newCSName, 1 );
            } else {
                RenameUtil.checkRenameCSResult( db, newCSName, csName, 1 );
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

    private class RenameCSAThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csName, newCSName );
            }
        }
    }

    private class RenameCSBThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Thread.sleep( new Random().nextInt( 100 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( newCSName, csName );
            }
        }
    }

}
