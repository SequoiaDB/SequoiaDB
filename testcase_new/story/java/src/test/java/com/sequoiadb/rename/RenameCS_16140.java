package com.sequoiadb.rename;

import java.util.Arrays;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCS_16140.java detach子表和修改主表cs名并发
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCS_16140 extends SdbTestBase {

    private String mainCSName = "maincs_16140";
    private String newMainCSName = "maincs_16140_new";
    private String subCSName = "subcs_16140";
    private String mainCLName = "maincl_16140";
    private String subCLName = "subcl_16140";
    private Sequoiadb sdb = null;
    private CollectionSpace mainCS = null;
    private CollectionSpace subCS = null;
    private DBCollection mainCL = null;
    private int clNum = 10;
    private int recordNum = 2000;
    private int detachNum = 0;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        mainCS = sdb.createCollectionSpace( mainCSName );
        subCS = sdb.createCollectionSpace( subCSName );
        createMainAndSubCL();
        RenameUtil.insertData( mainCL, recordNum );
    }

    @Test
    public void test() {
        RenameCSThread reCSNameThread = new RenameCSThread();
        DetachCLThread detachThread = new DetachCLThread();

        reCSNameThread.start();
        detachThread.start();

        boolean renameCS = reCSNameThread.isSuccess();
        boolean detachCL = detachThread.isSuccess();

        if ( !renameCS ) {
            Integer[] errnosA = { -147, -190 };
            BaseException errorA = ( BaseException ) reCSNameThread
                    .getExceptions().get( 0 );
            errorA.printStackTrace();
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( reCSNameThread.getErrorMsg() );
            }
        }

        if ( !detachCL ) {
            Integer[] errnosB = { -23, -34 };
            BaseException errorB = ( BaseException ) detachThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( detachThread.getErrorMsg() );
            }
        }

        if ( !renameCS && !detachCL ) {
            Assert.fail( "rename and detach cl all failed," + " rename msg: "
                    + reCSNameThread.getErrorMsg() + " detach msg: "
                    + detachThread.getErrorMsg() );
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( renameCS ) {
                RenameUtil.checkRenameCSResult( db, mainCSName, newMainCSName,
                        0 );
            } else {
                Assert.assertTrue( db.isCollectionSpaceExist( mainCSName ),
                        "rename mainCS failed" );
                Assert.assertFalse( db.isCollectionSpaceExist( newMainCSName ),
                        "rename mainCS failed" );
            }
            checkSnapshot( db, detachNum, renameCS );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, newMainCSName );
            CommLib.clearCS( sdb, subCSName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Thread.sleep( new Random().nextInt( 10 ) );
                db.renameCollectionSpace( mainCSName, newMainCSName );
            }
        }
    }

    private class DetachCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( mainCSName );
                DBCollection cl = cs.getCollection( mainCLName );
                for ( int i = 0; i < clNum; i++ ) {
                    cl.detachCollection(
                            subCSName + "." + subCLName + "_" + i );
                    detachNum++;
                }
            }
        }
    }

    private void createMainAndSubCL() {
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "IsMainCL", true );
        mainCL = mainCS.createCollection( mainCLName, options );
        for ( int i = 0; i < clNum; i++ ) {
            subCS.createCollection( subCLName + "_" + i );
            BSONObject bound = new BasicBSONObject();
            bound.put( "LowBound", new BasicBSONObject( "a", i * 200 ) );
            bound.put( "UpBound", new BasicBSONObject( "a", i * 200 + 200 ) );
            mainCL.attachCollection( subCSName + "." + subCLName + "_" + i,
                    bound );
        }
    }

    private void checkSnapshot( Sequoiadb db, int detachCLNum,
            boolean csRenameSuccess ) {
        String newMianFullName;
        if ( csRenameSuccess ) {
            newMianFullName = newMainCSName + "." + mainCLName;
        } else {
            newMianFullName = mainCSName + "." + mainCLName;
        }
        for ( int i = 0; i < clNum; i++ ) {
            DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{'Name':'" + subCSName + "." + subCLName + "_" + i + "'}",
                    "", "" );
            BSONObject obj = cur.getNext();
            if ( i < detachCLNum ) {
                if ( obj.toMap().containsKey( "MainCLName" ) ) {
                    Assert.fail( "cl aleary detach but found mainCL, snapshot:"
                            + obj.toString() );
                }
            } else {
                String mainCLName = ( String ) obj.get( "MainCLName" );
                Assert.assertEquals( mainCLName, newMianFullName,
                        "cl is not detach, but not found mainCL: "
                                + obj.toString() );
            }
        }
    }
}
