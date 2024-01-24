package com.sequoiadb.rename;

import java.util.Arrays;

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
 * @Description RenameCS_16141.java attach子表和修改子表cs名并发
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCS_16141 extends SdbTestBase {

    private String mainCSName = "maincs_16141";
    private String subCSName = "subcs_16141";
    private String newSubCSName = "subcs_16141_new";
    private String mainCLName = "maincl_16141";
    private String subCLName = "subcl_16141";
    private Sequoiadb sdb = null;
    private CollectionSpace mainCS = null;
    private CollectionSpace subCS = null;
    private int clNum = 10;
    private int attachNum = 0;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        mainCS = sdb.createCollectionSpace( mainCSName );
        subCS = sdb.createCollectionSpace( subCSName );
        createMainAndSubCL();
    }

    @Test
    public void test() {
        RenameCSThread reCSNameThread = new RenameCSThread();
        AttachCLThread atttachThread = new AttachCLThread();

        reCSNameThread.start();
        atttachThread.start();

        boolean renameCS = reCSNameThread.isSuccess();
        boolean attachCL = atttachThread.isSuccess();

        if ( !renameCS ) {
            Integer[] errnosA = { -147, -190 };
            BaseException errorA = ( BaseException ) reCSNameThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( reCSNameThread.getErrorMsg() );
            }
        }

        if ( !attachCL ) {
            Integer[] errnosB = { -23, -34, -147, -190 };
            BaseException errorB = ( BaseException ) atttachThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( atttachThread.getErrorMsg() );
            }
        }

        if ( !renameCS && !attachCL ) {
            Assert.fail( "rename and detach cl all failed," + " rename msg: "
                    + reCSNameThread.getErrorMsg() + " attach msg: "
                    + atttachThread.getErrorMsg() );
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( renameCS ) {
                RenameUtil.checkRenameCSResult( db, subCSName, newSubCSName,
                        0 );
            } else {
                Assert.assertTrue( db.isCollectionSpaceExist( subCSName ),
                        "rename Subcs failed" );
                Assert.assertFalse( db.isCollectionSpaceExist( newSubCSName ),
                        "rename Subcs failed" );
            }
            checkSnapshot( db, attachNum, renameCS );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, mainCSName );
            CommLib.clearCS( sdb, subCSName );
            CommLib.clearCS( sdb, newSubCSName );
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
                db.renameCollectionSpace( subCSName, newSubCSName );
            }
        }
    }

    private class AttachCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( mainCSName );
                DBCollection cl = cs.getCollection( mainCLName );
                for ( int i = 0; i < clNum; i++ ) {
                    BSONObject bound = new BasicBSONObject();
                    bound.put( "LowBound",
                            new BasicBSONObject( "a", i * 200 ) );
                    bound.put( "UpBound",
                            new BasicBSONObject( "a", i * 200 + 200 ) );
                    cl.attachCollection( subCSName + "." + subCLName + "_" + i,
                            bound );
                    attachNum++;
                }
            }
        }
    }

    private void createMainAndSubCL() {
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "IsMainCL", true );
        mainCS.createCollection( mainCLName, options );
        for ( int i = 0; i < clNum; i++ ) {
            subCS.createCollection( subCLName + "_" + i );
        }
    }

    private void checkSnapshot( Sequoiadb db, int attachCLNum,
            boolean csRenameSuccess ) {
        String mianFullName = mainCSName + "." + mainCLName;
        String chechSubCSName;
        if ( csRenameSuccess ) {
            chechSubCSName = newSubCSName;
        } else {
            chechSubCSName = subCSName;
        }
        for ( int i = 0; i < clNum; i++ ) {
            DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{'Name':'" + chechSubCSName + "." + subCLName + "_" + i
                            + "'}",
                    null, null );
            BSONObject obj = cur.getNext();
            if ( i < attachCLNum ) {
                if ( !obj.toMap().containsKey( "MainCLName" ) ) {
                    Assert.fail( "cl is attach, but not found mainCL: "
                            + obj.toString() );
                }
                Assert.assertEquals( obj.get( "MainCLName" ), mianFullName,
                        "chech mainCLName: " + obj.toString() );
            } else {
                if ( obj.toMap().containsKey( "MainCLName" ) ) {
                    Assert.fail( "cl is not attach but found mainCL, snapshot:"
                            + obj.toString() );
                }
            }
        }
    }

}
