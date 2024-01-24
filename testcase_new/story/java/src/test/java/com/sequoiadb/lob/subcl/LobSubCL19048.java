package com.sequoiadb.lob.subcl;

import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19048 :: 版本: 1 :: 并发去挂载子表和切分子表
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19048 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String mainCLName = "mainCL_19048";
    private String subCLName = "subCL_19048";
    private String sourceGroupName;
    private String destGroupName;
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;
    private List< ObjectId > lobIds;
    private int timeout = 600000;

    private CollectionSpace cs;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "can not support split" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        mainCL = cs.createCollection( mainCLName, options );

        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "AutoSplit", false );
        cs.createCollection( subCLName, clOptions );

        BSONObject bound = new BasicBSONObject();
        bound.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        bound.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        mainCL.attachCollection( SdbTestBase.csName + "." + subCLName, bound );

        sourceGroupName = RandomWriteLobUtil.getSrcGroupName( sdb,
                SdbTestBase.csName, subCLName );
        destGroupName = RandomWriteLobUtil.getSplitGroupName( sdb,
                sourceGroupName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        lobIds = LobSubUtils.createAndWriteLob( mainCL, lobBuff );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor( timeout );
        SplitCLThread splitCL = new SplitCLThread();
        thread.addWorker( new DetachCLThread() );
        thread.addWorker( splitCL );
        thread.run();

        DBCollection subCL = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( subCLName );
        LobSubUtils.checkLobMD5( subCL, lobIds, lobBuff );
        if ( splitCL.getRetCode() != 0 ) {
            subCL.split( sourceGroupName, destGroupName, 50 );
        }
        LobSubUtils.checkLobMD5( subCL, lobIds, lobBuff );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DetachCLThread {

        @ExecuteOrder(step = 1)
        private void detachCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                try {
                    Thread.sleep( new Random().nextInt( 100 ) );
                } catch ( InterruptedException e ) {
                }
                mainCL.detachCollection( SdbTestBase.csName + "." + subCLName );
            }
        }
    }

    private class SplitCLThread extends ResultStore {

        @ExecuteOrder(step = 1)
        private void splitCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subCL = db.getCollectionSpace( csName )
                        .getCollection( subCLName );
                subCL.split( sourceGroupName, destGroupName, 50 );
            } catch ( BaseException e ) {
                e.printStackTrace();
                saveResult( e.getErrorCode(), e );
            }
        }
    }

}
