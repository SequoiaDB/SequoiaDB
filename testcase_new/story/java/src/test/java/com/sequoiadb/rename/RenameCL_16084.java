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
 * @Description RenameCL_16084.java 并发修改cl名和创建相同cl
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_16084 extends SdbTestBase {

    private String clName = "rename_CL_16084";
    private String newCLName = "rename_CL_16084_new";
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
        CreateCLThread createCLThread = new CreateCLThread();

        renameCLThread.start();
        createCLThread.start();

        boolean renameCL = renameCLThread.isSuccess();
        boolean createCL = createCLThread.isSuccess();
        Assert.assertTrue( renameCL, renameCLThread.getErrorMsg() );

        if ( !createCL ) {
            Integer[] errnos = { -22 };
            BaseException error = ( BaseException ) createCLThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( createCLThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( renameCL && !createCL ) {
                RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
            } else if ( renameCL && createCL ) {
                cs = db.getCollectionSpace( SdbTestBase.csName );
                if ( !cs.isCollectionExist( clName ) ) {
                    Assert.fail( "cl is been create, should exist" );
                }
                if ( !cs.isCollectionExist( newCLName ) ) {
                    Assert.fail( "cl is been rename, should exist" );
                }
                checkCLRecord( db, clName, 0,
                        "The cl is newly created, should be empty" );
                checkCLRecord( db, newCLName, recordNum,
                        "The cl is rename, should have record" );
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
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.renameCollection( clName, newCLName );
            }
        }
    }

    private class CreateCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.createCollection( clName );
            }
        }
    }

    private void checkCLRecord( Sequoiadb db, String clName, int recordNum,
            String msg ) {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        long actNum = dbcl.getCount();
        Assert.assertEquals( actNum, recordNum, msg );
    }

}
