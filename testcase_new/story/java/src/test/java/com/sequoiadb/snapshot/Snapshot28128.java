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
 * @Description seqDB-28128:写lob数据存在空洞 ，查看快照中TotalLobSize和TotalValidLobSize指标
 * @Author liuli
 * @Date 2022.10.05
 * @UpdateAuthor liuli
 * @UpdateDate 2022.10.05
 * @version 1.10
 */
public class Snapshot28128 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private String csName = "cs_28128";
    private String clName = "cl_28128";
    private int lobPageSize = 262144;
    private ArrayList< String > groupNames = new ArrayList<>();
    private int lobSize = 2000;

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
        // 创建一个lob
        DBLob lob = dbcl.createLob();
        ObjectId lobid = lob.getID();
        lob.close();

        int seekSize = 262144;
        int nodeName = CommLib.getNodeAddress( sdb, groupNames.get( 0 ) )
                .size();
        int lobPages = ( lobSize + seekSize ) / lobPageSize + 1;
        Node masterNode = sdb.getReplicaGroup( groupNames.get( 0 ) )
                .getMaster();
        String masterNodeName = masterNode.getNodeName();

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

        // 已SDB_LOB_WRITE模式打开lob
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        lob = dbcl.openLob( lobid, DBLob.SDB_LOB_WRITE );

        // 偏移
        lob.seek( seekSize, DBLob.SDB_LOB_SEEK_SET );

        // SDB_LOB_WRITE模式写lob
        lob.write( lobBuff );
        lob.close();

        // 校验数据库快照
        // Number of load monitoring nodes
        int loadMonitorNodeNum = 2;
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" )
                        + 1 * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages );
        dataInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * loadMonitorNodeNum );
        dataInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWrite" ) + 2 );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPages );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages );

        // 获取集合空间快照信息
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + 1 );
        csInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) csInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages );
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize );
        csInfos.get( 0 ).put( "TotalLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobSize" )
                        + ( lobSize + 1024 ) * nodeName );
        csInfos.get( 0 ).put( "TotalValidLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        + ( lobSize + seekSize ) * nodeName );
        csInfos.get( 0 ).put( "TotalLobPages", ( double ) lobPages * nodeName );
        csInfos.get( 0 ).put( "TotalUsedLobSpace",
                ( double ) lobPages * lobPageSize * nodeName );
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
        csInfos.get( 0 ).put( "AvgLobSize", ( long ) lobSize + seekSize );
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
                clInfo.put( "TotalLobAddressing",
                        ( long ) clInfo.get( "TotalLobAddressing" ) + lobPages
                                + 1 );
                clInfo.put( "TotalLobWriteSize",
                        ( long ) clInfo.get( "TotalLobWriteSize" ) + lobSize );
                clInfo.put( "TotalLobWrite",
                        ( long ) clInfo.get( "TotalLobWrite" )
                                + lobPageSize * 2L );
                clInfo.put( "TotalLobRead",
                        ( long ) clInfo.get( "TotalLobRead" )
                                + lobPageSize * 2L );
            }
            clInfo.put( "TotalLobSize", ( long ) clInfo.get( "TotalLobSize" )
                    + ( long ) lobSize + 1024 );
            clInfo.put( "TotalValidLobSize",
                    ( long ) clInfo.get( "TotalValidLobSize" )
                            + ( long ) lobSize + seekSize );
            clInfo.put( "TotalLobPages", lobPages );
            clInfo.put( "TotalUsedLobSpace", ( long ) lobPages * lobPageSize );
            double lobUsageRate = ( long ) clInfo.get( "TotalLobSize" )
                    / ( long ) clInfo.get( "TotalUsedLobSpace" );
            clInfo.put( "LobUsageRate", lobUsageRate );
            clInfo.put( "FreeLobSize", clInfo.get( "FreeLobSpace" ) );
            clInfo.put( "AvgLobSize",
                    ( long ) clInfo.get( "TotalValidLobSize" ) );
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
