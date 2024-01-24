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
 * @Description seqDB-27870:同一CL下并发执行putLob/getLob/deleteLob
 * @Author huanghaimei
 * @Date 2022.11.03
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.02.08
 */
public class Snapshot27870 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private String csName = "cs_27870";
    private String clName = "cl_27870";
    private int lobPageSize = 262144;
    private ArrayList< String > groupNames = new ArrayList<>();
    private int lobSize = 1024 * 200;
    private byte[] lobBuff;
    ArrayList< ObjectId > lobids1 = new ArrayList<>();
    ArrayList< ObjectId > lobids2 = new ArrayList<>();
    int lobs = 10;

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
        dbcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );

        for ( int i = 0; i < lobs; i++ ) {
            DBLob lob = dbcl.createLob();
            lob.write( lobBuff );
            lob.close();
            ObjectId lobid = lob.getID();
            lobids1.add( lobid );
        }
        for ( int i = 0; i < lobs; i++ ) {
            DBLob lob = dbcl.createLob();
            lob.write( lobBuff );
            lob.close();
            ObjectId lobid = lob.getID();
            lobids2.add( lobid );
        }
    }

    @Test
    public void testLob() throws Exception {
        // 获取数据库快照信息
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        ArrayList< BSONObject > dataInfos = SnapshotUtil
                .getSnapshotLobStat( cursor );

        // 获取集合空间快照信息
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject( "Name", csName ), null, null );
        ArrayList< BSONObject > csInfos = SnapshotUtil
                .getSnapshotLobStat( cursor );

        ThreadExecutor th = new ThreadExecutor( 180000 );

        for ( int i = 0; i < lobs; i++ ) {
            th.addWorker( new PutLob() );
        }

        th.addWorker( new DeleteLob( lobids1 ) );
        th.addWorker( new GetLob( lobids2 ) );
        th.run();

        int srcNodeNum = CommLib.getNodeAddress( sdb, groupNames.get( 0 ) )
                .size();
        int dstNodeNum = CommLib.getNodeAddress( sdb, groupNames.get( 1 ) )
                .size();
        int lobPages = ( lobSize + 1023 ) / lobPageSize + 1;

        // 校验数据库快照
        int loadMonitorNodeNum = 2;
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" )
                        + lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobGet",
                ( double ) dataInfos.get( 0 ).get( "TotalLobGet" )
                        + lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobDelete",
                ( double ) dataInfos.get( 0 ).get( "TotalLobDelete" )
                        + lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages * lobs * loadMonitorNodeNum * 2 );
        dataInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * lobs * loadMonitorNodeNum * 3 );
        dataInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobSize * lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobs * lobPages );
        dataInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobs * lobPages * 3 );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobs * lobPages * 3 );
        dataInfos.get( 0 ).put( "TotalLobTruncate",
                ( double ) dataInfos.get( 0 ).get( "TotalLobTruncate" )
                        + lobs * lobPages );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages * lobs );

        // 获取集合空间快照信息
        csInfos.get( 0 ).put( "LobCapacity", ( double ) SnapshotUtil.lobdSize
                * ( srcNodeNum + dstNodeNum ) );
        csInfos.get( 0 ).put( "LobMetaCapacity",
                ( double ) SnapshotUtil.lobmSize
                        * ( srcNodeNum + dstNodeNum ) );
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + lobs );
        csInfos.get( 0 ).put( "TotalLobGet",
                ( double ) csInfos.get( 0 ).get( "TotalLobGet" ) + lobs );
        csInfos.get( 0 ).put( "TotalLobDelete",
                ( double ) csInfos.get( 0 ).get( "TotalLobDelete" ) + lobs );
        csInfos.get( 0 ).put( "TotalLobTruncate",
                ( double ) csInfos.get( 0 ).get( "TotalLobTruncate" ) + lobs );
        csInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) csInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages * lobs * 4 );
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * lobs * 3 );
        csInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobSize * lobs );
        csInfos.get( 0 ).put( "LobUsageRate",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        / ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "FreeLobSpace",
                ( double ) csInfos.get( 0 ).get( "LobCapacity" )
                        - ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "FreeLobSize",
                ( double ) csInfos.get( 0 ).get( "FreeLobSpace" ) );
        csInfos.get( 0 ).put( "AvgLobSize", ( long ) lobSize );
        csInfos.get( 0 ).put( "TotalLobRead",
                ( double ) csInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPageSize * 2 );
        csInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) csInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobPageSize * 2 );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject( "Name", csName ), null, null );
        SnapshotUtil.checkSnapshot( cursor, csInfos, lobPages * lobs );
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

    private class DeleteLob extends ResultStore {
        private ArrayList< ObjectId > lobids;

        public DeleteLob( ArrayList< ObjectId > lobids ) {
            this.lobids = lobids;
        }

        @ExecuteOrder(step = 1)
        public void removeLob() throws InterruptedException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( ObjectId lobid : lobids ) {
                    dbcl.removeLob( lobid );
                }
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

    private class PutLob extends ResultStore {

        @ExecuteOrder(step = 1)
        public void putLob()
                throws InterruptedException, NoSuchAlgorithmException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                DBLob lob = dbcl.createLob();
                lob.write( lobBuff );
                lob.close();
            }
        }
    }
}