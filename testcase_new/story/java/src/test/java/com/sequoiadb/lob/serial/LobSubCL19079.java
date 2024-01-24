package com.sequoiadb.lob.serial;

import java.util.List;

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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19079 :: 版本: 1 :: 并发rename主表集合空间名和读写删lob
 * @author luweikang
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19079 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String oldMainCSName = "mainCS_19079old";
    private String newMainCSName = "mainCS_19079new";
    private String mainCLName = "mainCL_19079";
    private String subCSName = "subCS_19079";
    private String subCLName = "subCL_19079";
    private DBCollection mainCL = null;
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;
    private List< ObjectId > lobIds1;
    private List< ObjectId > lobIds2;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        CollectionSpace cs = sdb.createCollectionSpace( oldMainCSName );
        CollectionSpace subCS = sdb.createCollectionSpace( subCSName );
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        mainCL = cs.createCollection( mainCLName, options );

        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "AutoSplit", true );
        subCS.createCollection( subCLName, clOptions );

        BSONObject bound = new BasicBSONObject();
        bound.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        bound.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        mainCL.attachCollection( subCSName + "." + subCLName, bound );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
        lobIds1 = LobSubUtils.createAndWriteLob( mainCL, lobBuff );
        lobIds2 = LobSubUtils.createAndWriteLob( mainCL, lobBuff );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor( 600000 );
        thread.addWorker( new ReadLobThread() );
        thread.addWorker( new RemoveLobThread() );
        thread.addWorker( new PutLobThread() );
        thread.addWorker( new RenameThread() );
        thread.run();

        mainCL = sdb.getCollectionSpace( newMainCSName )
                .getCollection( mainCLName );
        LobSubUtils.checkLobMD5( mainCL, lobIds1, lobBuff );
    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( oldMainCSName ) ) {
                sdb.dropCollectionSpace( oldMainCSName );
            }
            if ( sdb.isCollectionSpaceExist( newMainCSName ) ) {
                sdb.dropCollectionSpace( newMainCSName );
            }
            if ( sdb.isCollectionSpaceExist( subCSName ) ) {
                sdb.dropCollectionSpace( subCSName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class ReadLobThread {

        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                LobSubUtils.checkLobMD5( mainCL, lobIds1, lobBuff );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

    private class RemoveLobThread {

        @ExecuteOrder(step = 1)
        private void removeLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                for ( ObjectId lobId : lobIds2 ) {
                    mainCL.removeLob( lobId );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

    private class PutLobThread {

        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                LobSubUtils.createAndWriteLob( mainCL, lobBuff );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

    private class RenameThread {

        @ExecuteOrder(step = 1)
        private void renameCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                }
                db.renameCollectionSpace( oldMainCSName, newMainCSName );
            }
        }
    }

}
