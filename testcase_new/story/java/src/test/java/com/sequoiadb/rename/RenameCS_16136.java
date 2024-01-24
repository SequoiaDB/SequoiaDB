package com.sequoiadb.rename;

import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description RenameCS_16136.java 主子表在不同cs上，并发修改主表和子表的cs名
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCS_16136 extends SdbTestBase {

    private String mainCSName = "maincs_16136";
    private String mainCLName = "maincl_16136";
    private String newMainCSName = "maincs_16136_new";
    private String subCSName = "subcs_16136";
    private String subCLName = "subcl_16136";
    private String newSubCSName = "subcs_16136_new";
    private Sequoiadb sdb = null;
    private CollectionSpace mainCS = null;
    private CollectionSpace subCS = null;
    private DBCollection mainCL = null;
    private int recordNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        mainCS = sdb.createCollectionSpace( mainCSName );
        subCS = sdb.createCollectionSpace( subCSName );
        createMainAndSubCL();
        RenameUtil.insertData( mainCL, recordNum );
    }

    @Test
    public void test() {
        RenameMainCSThread reMainCSNameThread = new RenameMainCSThread();
        RenameSubCSThread reSubCSNameThread = new RenameSubCSThread();

        reMainCSNameThread.start();
        reSubCSNameThread.start();

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( !reMainCSNameThread.isSuccess() ) {
                Integer[] errnosB = { -148 };
                BaseException errorB = ( BaseException ) reMainCSNameThread
                        .getExceptions().get( 0 );
                if ( !Arrays.asList( errnosB )
                        .contains( errorB.getErrorCode() ) ) {
                    Assert.fail( reMainCSNameThread.getErrorMsg() );
                }
                RenameUtil.retryToRenameCS( db, mainCSName, newMainCSName );
            } else {
                RenameUtil.checkRenameCSResult( db, mainCSName, newMainCSName,
                        0 );
            }

            if ( !reSubCSNameThread.isSuccess() ) {
                Integer[] errnosB = { -148 };
                BaseException errorB = ( BaseException ) reSubCSNameThread
                        .getExceptions().get( 0 );
                if ( !Arrays.asList( errnosB )
                        .contains( errorB.getErrorCode() ) ) {
                    Assert.fail( reSubCSNameThread.getErrorMsg() );
                }
                RenameUtil.retryToRenameCS( db, subCSName, newSubCSName );
            } else {
                RenameUtil.checkRenameCSResult( db, subCSName, newSubCSName,
                        1 );
            }
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

    private class RenameMainCSThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Thread.sleep( 5 );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( mainCSName, newMainCSName );
            }
        }
    }

    private class RenameSubCSThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( subCSName, newSubCSName );
            }
        }
    }

    private void createMainAndSubCL() {
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "IsMainCL", true );
        mainCL = mainCS.createCollection( mainCLName, options );
        subCS.createCollection( subCLName );
        BSONObject bound = new BasicBSONObject();
        bound.put( "LowBound", new BasicBSONObject( "a", 0 ) );
        bound.put( "UpBound", new BasicBSONObject( "a", 2000 ) );
        mainCL.attachCollection( subCSName + "." + subCLName, bound );
    }
}
