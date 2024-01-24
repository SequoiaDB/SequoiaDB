package com.sequoiadb.snapshot;

import java.util.ArrayList;

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
 * @version 1.10
 * @Description seqDB-27862:SDB_LOB_SHAREREAD|WRITE模式写数据
 * @Author liuli
 * @Date 2022.09.29
 * @UpdateAuthor liuli
 * @UpdateDate 2022.09.29
 */
public class Snapshot27862 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private String csName = "cs_27862";
    private String clName = "cl_27862";
    private int lobPageSize = 262144;
    private ArrayList< String > groupNames = new ArrayList<>();
    private int lobSize = 1024 * 1024;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        dbcs = sdb.createCollectionSpace( csName,
                new BasicBSONObject( "LobPageSize", lobPageSize ) );
        dbcl = dbcs.createCollection( clName,
                new BasicBSONObject( "Group", groupNames.get( 0 ) )
                        .append( "ReplSize", -1 ) );
    }

    @Test
    public void testLob() {
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

        // 获取集合快照
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        ArrayList< BSONObject > clInfos = SnapshotUtil
                .getSnapshotLobStatCL( cursor, false );

        // 创建一个lob
        ObjectId lobid = dbcl.createLobID();
        DBLob lob = dbcl.createLob( lobid );
        lob.close();

        int nodeNum = CommLib.getNodeAddress( sdb, groupNames.get( 0 ) ).size();
        int lobPages = ( lobSize + 1023 ) / lobPageSize + 1;
        Node masterNode = sdb.getReplicaGroup( groupNames.get( 0 ) )
                .getMaster();
        String masterNodeName = masterNode.getNodeName();
        // 校验数据库
        // Number of load monitoring nodes
        int loadMonitorNodeNum = 2;
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" )
                        + 1 * loadMonitorNodeNum );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages * 2 );
        // 检验集合空间
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + 1 );
        csInfos.get( 0 ).put( "LobCapacity",
                ( double ) csInfos.get( 0 ).get( "LobCapacity" )
                        + 1.34217728E8 * nodeNum );
        csInfos.get( 0 ).put( "TotalLobs",
                ( double ) csInfos.get( 0 ).get( "TotalLobs" ) + 1 * nodeNum );
        csInfos.get( 0 ).put( "TotalLobPages",
                ( double ) csInfos.get( 0 ).get( "TotalLobPages" )
                        + 1 * nodeNum );
        csInfos.get( 0 ).put( "LobUsageRate",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        / ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "TotalUsedLobSpace",
                ( double ) csInfos.get( 0 ).get( "TotalUsedLobSpace" )
                        + lobPageSize * nodeNum );
        csInfos.get( 0 ).put( "FreeLobSpace",
                ( double ) csInfos.get( 0 ).get( "LobCapacity" )
                        - ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "FreeLobSize",
                ( double ) csInfos.get( 0 ).get( "FreeLobSpace" ) );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject( "Name", csName ), null, null );
        SnapshotUtil.checkSnapshot( cursor, csInfos, lobPages * 2 );
        // 检验集合快照
        for ( BSONObject clInfo : clInfos ) {
            if ( clInfo.get( "NodeName" ).equals( masterNodeName ) ) {
                clInfo.put( "TotalLobPut",
                        ( long ) clInfo.get( "TotalLobPut" ) + 1 );

                clInfo.put( "TotalLobWriteSize",
                        ( long ) clInfo.get( "TotalLobWriteSize" ) + lobSize );
                clInfo.put( "TotalLobWrite",
                        ( long ) clInfo.get( "TotalLobWrite" )
                                + lobPageSize * 2L );
                clInfo.put( "TotalLobRead",
                        ( long ) clInfo.get( "TotalLobRead" )
                                + lobPageSize * 2L );
            }
            clInfo.put( "TotalLobs", ( long ) clInfo.get( "TotalLobs" ) + 1 );
            clInfo.put( "TotalLobPages", 1 );
            clInfo.put( "TotalUsedLobSpace",
                    ( long ) clInfo.get( "TotalUsedLobSpace" ) + lobPageSize );
        }
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        SnapshotUtil.checkSnapshotCL( cursor, clInfos, false, lobPages * 2 );

        // 已SDB_LOB_SHAREREAD|WRITE模式打开lob
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        lob = dbcl.openLob( lobid,
                DBLob.SDB_LOB_WRITE | DBLob.SDB_LOB_SHAREREAD );
        // 校验数据库快照
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages );

        // 获取集合空间快照信息
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject( "Name", csName ), null, null );
        SnapshotUtil.checkSnapshot( cursor, csInfos, lobPages );

        // 获取集合快照
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        SnapshotUtil.checkSnapshotCL( cursor, clInfos, false, lobPages );

        // SDB_LOB_WRITE模式写lob
        lob.write( lobBuff );
        lob.close();

        // 校验数据库快照
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" )
                        + 1 * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobGet",
                ( double ) dataInfos.get( 0 ).get( "TotalLobGet" )
                        + 1 * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages );
        dataInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWrite" )
                        + loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPages );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages * 2 );

        // 获取集合空间快照信息
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + 1 );
        csInfos.get( 0 ).put( "TotalLobGet",
                ( double ) csInfos.get( 0 ).get( "TotalLobGet" ) + 1 );
        csInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) csInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages );
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize );
        csInfos.get( 0 ).put( "TotalLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobSize" )
                        + lobSize * nodeNum );
        csInfos.get( 0 ).put( "TotalValidLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        + lobSize * nodeNum );
        csInfos.get( 0 ).put( "TotalLobPages", ( double ) lobPages * nodeNum );
        csInfos.get( 0 ).put( "LobUsageRate",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        / ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "TotalUsedLobSpace",
                ( double ) lobPages * lobPageSize * nodeNum );
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
        SnapshotUtil.checkSnapshot( cursor, csInfos, lobPages * 2 );

        // 获取集合快照
        for ( BSONObject clInfo : clInfos ) {
            if ( clInfo.get( "NodeName" ).equals( masterNodeName ) ) {
                clInfo.put( "TotalLobPut",
                        ( long ) clInfo.get( "TotalLobPut" ) + 1 );
                clInfo.put( "TotalLobGet",
                        ( long ) clInfo.get( "TotalLobGet" ) + 1 );
                clInfo.put( "TotalLobAddressing",
                        ( long ) clInfo.get( "TotalLobAddressing" )
                                + lobPages );
                clInfo.put( "TotalLobWriteSize",
                        ( long ) clInfo.get( "TotalLobWriteSize" ) + lobSize );
                clInfo.put( "TotalLobWrite",
                        ( long ) clInfo.get( "TotalLobWrite" )
                                + lobPageSize * 2L );
                clInfo.put( "TotalLobRead",
                        ( long ) clInfo.get( "TotalLobRead" )
                                + lobPageSize * 2L );
            }
            clInfo.put( "TotalLobSize",
                    ( long ) clInfo.get( "TotalLobSize" ) + ( long ) lobSize );
            clInfo.put( "TotalUsedLobSpace", ( long ) lobPages * lobPageSize );
            clInfo.put( "TotalValidLobSize",
                    ( long ) clInfo.get( "TotalValidLobSize" )
                            + ( long ) lobSize );
            clInfo.put( "TotalLobPages", lobPages );
            double lobUsageRate = ( long ) clInfo.get( "TotalLobSize" )
                    / ( long ) clInfo.get( "TotalUsedLobSpace" );
            clInfo.put( "LobUsageRate", lobUsageRate );
            clInfo.put( "FreeLobSize", clInfo.get( "FreeLobSpace" ) );
            clInfo.put( "AvgLobSize", ( long ) lobSize );
        }
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        SnapshotUtil.checkSnapshotCL( cursor, clInfos, false, lobPages * 2 );

        lob = dbcl.openLob( lobid );
        lob.getSize();
        lob.getCreateTime();
        lob.close();

        // 校验数据库快照
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" ) );
        dataInfos.get( 0 ).put( "TotalLobGet",
                ( double ) dataInfos.get( 0 ).get( "TotalLobGet" )
                        + loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" ) );
        dataInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWrite" )
                        + loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobSize * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPages );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages * 2 );

        // 获取集合空间快照信息
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize );
        csInfos.get( 0 ).put( "TotalLobGet",
                ( double ) csInfos.get( 0 ).get( "TotalLobGet" ) + 1 );
        csInfos.get( 0 ).put( "TotalLobPages",
                csInfos.get( 0 ).get( "TotalLobPages" ) );
        csInfos.get( 0 ).put( "LobUsageRate",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        / ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "TotalUsedLobSpace",
                csInfos.get( 0 ).get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "FreeLobSpace",
                ( double ) csInfos.get( 0 ).get( "LobCapacity" )
                        - ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "FreeLobSize",
                ( double ) csInfos.get( 0 ).get( "FreeLobSpace" ) );
        csInfos.get( 0 ).put( "TotalLobRead",
                ( double ) csInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPageSize * 2 );
        csInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) csInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobPageSize * 2 );
        csInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobSize );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject( "Name", csName ), null, null );
        SnapshotUtil.checkSnapshot( cursor, csInfos, lobPages * 2 );

        // 获取集合快照
        for ( BSONObject clInfo : clInfos ) {
            if ( clInfo.get( "NodeName" ).equals( masterNodeName ) ) {
                clInfo.put( "TotalLobWriteSize",
                        ( long ) clInfo.get( "TotalLobWriteSize" ) + lobSize );
                clInfo.put( "TotalLobWrite",
                        ( long ) clInfo.get( "TotalLobWrite" )
                                + lobPageSize * 2L );
                clInfo.put( "TotalLobRead",
                        ( long ) clInfo.get( "TotalLobRead" )
                                + lobPageSize * 2L );
                clInfo.put( "TotalLobGet",
                        ( long ) clInfo.get( "TotalLobGet" ) + 1 );
                clInfo.put( "TotalLobReadSize",
                        ( long ) clInfo.get( "TotalLobReadSize" ) + lobSize );
            }
            clInfo.put( "FreeLobSize", clInfo.get( "FreeLobSpace" ) );
        }
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        SnapshotUtil.checkSnapshotCL( cursor, clInfos, false, lobPages * 2 );
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
}