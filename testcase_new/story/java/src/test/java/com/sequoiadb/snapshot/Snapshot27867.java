package com.sequoiadb.snapshot;

import com.sequoiadb.base.*;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;

/**
 * @version 1.10
 * @Description seqDB-27867:同一CL下并发执行getlob操作和split
 * @Author huanghaimei
 * @Date 2022.11.02
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.02.08
 */
public class Snapshot27867 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private String csName = "cs_27867";
    private String clName = "cl_27867";
    private int lobPageSize = 262144;
    private ArrayList< String > groupNames = new ArrayList<>();
    private int lobSize = 1024 * 200;
    private byte[] lobBuff;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "is one group skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        dbcs = sdb.createCollectionSpace( csName,
                new BasicBSONObject( "LobPageSize", lobPageSize ) );
        BasicBSONObject option = new BasicBSONObject();
        option.append( "Group", groupNames.get( 0 ) );
        option.append( "ReplSize", -1 );
        option.append( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        dbcl = dbcs.createCollection( clName, option );
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
    }

    @Test
    public void testLob() throws Exception {
        // 获取数据库快照信息
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        ArrayList< BSONObject > dataInfos = SnapshotUtil
                .getSnapshotLobStat( cursor );
        int lobs = 50;
        // 执行putlob操作
        ArrayList< ObjectId > lobids = new ArrayList<>();
        for ( int i = 0; i < lobs; i++ ) {
            DBLob lob = dbcl.createLob();
            lob.write( lobBuff );
            lob.close();
            ObjectId lobid = lob.getID();
            lobids.add( lobid );
        }
        // 并发执行getLob和split
        ThreadExecutor th = new ThreadExecutor( 180000 );
        th.addWorker( new GetLob( lobids ) );
        th.addWorker( new Split() );
        th.run();

        int lobPages = ( lobSize + 1023 ) / lobPageSize + 1;

        // Number of load monitoring nodes
        int loadMonitorNodeNum = 2;
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" )
                        + lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobGet",
                ( double ) dataInfos.get( 0 ).get( "TotalLobGet" )
                        + lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobSize * lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages * lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobs * lobPages );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobs * lobPages );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages * lobs );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class Split extends ResultStore {

        @ExecuteOrder(step = 1)
        public void split() throws InterruptedException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 100 );
            }
        }
    }

    private class GetLob extends ResultStore {
        private ArrayList< ObjectId > lobids;

        public GetLob( ArrayList< ObjectId > lobids ) {
            this.lobids = lobids;
        }

        @ExecuteOrder(step = 1)
        public void getLob()
                throws InterruptedException, NoSuchAlgorithmException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( ObjectId lobid : lobids ) {
                    DBLob lob = dbcl.openLob( lobid );
                    lob.read( lobBuff );
                    lob.close();
                }
            }
        }
    }
}