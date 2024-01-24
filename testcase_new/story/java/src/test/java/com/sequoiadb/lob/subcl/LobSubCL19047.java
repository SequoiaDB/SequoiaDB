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
 * @Description seqDB-19047 :: 版本: 1 :: 并发挂载子表和切分子表
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19047 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String mainCLName = "mainCL_19047";
    private String subCLName = "subCL_19047";
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
        DBCollection subCL = cs.createCollection( subCLName, clOptions );

        sourceGroupName = RandomWriteLobUtil.getSrcGroupName( sdb,
                SdbTestBase.csName, subCLName );
        destGroupName = RandomWriteLobUtil.getSplitGroupName( sdb,
                sourceGroupName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        lobIds = LobSubUtils.createAndWriteLob( subCL, lobBuff );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor( timeout );
        SplitCLThread splitCL = new SplitCLThread();
        thread.addWorker( new AttachCLThread() );
        thread.addWorker( splitCL );
        thread.run();

        LobSubUtils.checkLobMD5( mainCL, lobIds, lobBuff );
        if ( splitCL.getRetCode() != 0 ) {
            DBCollection subCL = sdb.getCollectionSpace( csName )
                    .getCollection( subCLName );
            subCL.split( sourceGroupName, destGroupName, 50 );
        }
        LobSubUtils.checkLobMD5( mainCL, lobIds, lobBuff );
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

    private class AttachCLThread {

        @ExecuteOrder(step = 1)
        private void attachCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                BSONObject bound = new BasicBSONObject();
                bound.put( "LowBound",
                        new BasicBSONObject( "date", new MinKey() ) );
                bound.put( "UpBound",
                        new BasicBSONObject( "date", new MaxKey() ) );
                try {
                    Thread.sleep( new Random().nextInt( 100 ) );
                } catch ( InterruptedException e ) {
                }
                mainCL.attachCollection( SdbTestBase.csName + "." + subCLName,
                        bound );
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
                saveResult( e.getErrorCode(), e );
            }
        }
    }

}
