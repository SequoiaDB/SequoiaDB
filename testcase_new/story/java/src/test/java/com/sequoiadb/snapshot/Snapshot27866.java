package com.sequoiadb.snapshot;

import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;

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

import com.sequoiadb.base.*;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-27866:同一CL下并发执行putlob操作和split
 * @Author liuli
 * @Date 2022.10.17
 * @UpdateAuthor liuli
 * @UpdateDate 2022.10.17
 * @version 1.10
 */
public class Snapshot27866 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private String csName = "cs_27866";
    private String clName = "cl_27866";
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
        dbcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
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

        // 并发执行20次putLob和split
        ThreadExecutor th = new ThreadExecutor( 180000 );
        int lobs = 20;
        for ( int i = 0; i < lobs; i++ ) {
            th.addWorker( new PutLob() );
        }
        th.addWorker( new Split() );
        th.run();

        int srcNodeName = CommLib.getNodeAddress( sdb, groupNames.get( 0 ) )
                .size();
        int dstNodeName = CommLib.getNodeAddress( sdb, groupNames.get( 1 ) )
                .size();
        int lobPages = ( lobSize + 1023 ) / lobPageSize + 1;
        Node masterNode = sdb.getReplicaGroup( groupNames.get( 0 ) )
                .getMaster();
        String masterNodeName = masterNode.getNodeName();
        Node node = sdb.getReplicaGroup( groupNames.get( 0 ) ).getMaster();
        String hoseName = node.getHostName();
        int port = node.getPort();
        Sequoiadb data = new Sequoiadb( hoseName, port, "", "" );

        // 获取原组上lob数量
        int dataLobs = 0;
        cursor = data.getCollectionSpace( csName ).getCollection( clName )
                .listLobs();
        while ( cursor.hasNext() ) {
            cursor.getNext();
            dataLobs++;
        }
        cursor.close();

        // 校验数据库快照
        // Number of load monitoring nodes
        int loadMonitorNodeNum = 2;
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" )
                        + lobs * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages * lobs );
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

        // 获取集合空间快照信息
        csInfos.get( 0 ).put( "LobCapacity", ( double ) SnapshotUtil.lobdSize
                * ( srcNodeName + dstNodeName ) );
        csInfos.get( 0 ).put( "LobMetaCapacity",
                ( double ) SnapshotUtil.lobmSize
                        * ( srcNodeName + dstNodeName ) );
        csInfos.get( 0 ).put( "TotalLobs",
                ( double ) csInfos.get( 0 ).get( "TotalLobs" )
                        + dataLobs * srcNodeName
                        + ( lobs - dataLobs ) * dstNodeName );
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + lobs );
        csInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) csInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages * lobs );
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * lobs );
        csInfos.get( 0 ).put( "TotalLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobSize" )
                        + lobSize * dataLobs * srcNodeName
                        + lobSize * ( lobs - dataLobs ) * dstNodeName );
        csInfos.get( 0 ).put( "TotalValidLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        + lobSize * dataLobs * srcNodeName
                        + lobSize * ( lobs - dataLobs ) * dstNodeName );
        csInfos.get( 0 ).put( "TotalLobPages",
                ( double ) csInfos.get( 0 ).get( "TotalLobPages" )
                        + lobPages * dataLobs * srcNodeName
                        + lobPages * ( lobs - dataLobs ) * dstNodeName );
        csInfos.get( 0 ).put( "TotalUsedLobSpace",
                ( double ) csInfos.get( 0 ).get( "TotalUsedLobSpace" )
                        + lobPages * lobPageSize * dataLobs * srcNodeName
                        + lobPages * lobPageSize * ( lobs - dataLobs )
                                * dstNodeName );
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

    private class Split extends ResultStore {

        @ExecuteOrder(step = 1)
        public void split() throws InterruptedException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
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