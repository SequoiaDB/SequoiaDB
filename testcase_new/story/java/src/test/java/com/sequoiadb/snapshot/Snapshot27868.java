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
 * @Description seqDB-27868:同一CS下不同CL下并发执行putlob操作和split
 * @Author huanghaimei
 * @Date 2022.11.03
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.02.08
 */
public class Snapshot27868 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private String csName = "cs_27868";
    private String clName1 = "cl_27868_1";
    private String clName2 = "cl_27868_2";
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
        DBCollection dbcl1 = dbcs.createCollection( clName1, option );
        DBCollection dbcl2 = dbcs.createCollection( clName2, option );
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBLob lob1 = dbcl1.createLob();
        lob1.write( lobBuff );
        lob1.close();
        DBLob lob2 = dbcl2.createLob();
        lob2.write( lobBuff );
        lob2.close();
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

        int srcNodeNum = CommLib.getNodeAddress( sdb, groupNames.get( 0 ) )
                .size();
        int dstNodeNum = CommLib.getNodeAddress( sdb, groupNames.get( 1 ) )
                .size();
        int lobPages = ( lobSize + 1023 ) / lobPageSize + 1;

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
                * ( srcNodeNum + dstNodeNum ) );
        csInfos.get( 0 ).put( "LobMetaCapacity",
                ( double ) SnapshotUtil.lobmSize
                        * ( srcNodeNum + dstNodeNum ) );
        csInfos.get( 0 ).put( "MaxLobCapacity", ( double ) SnapshotUtil.lobmSize
                * ( srcNodeNum + dstNodeNum ) * lobPageSize );
        csInfos.get( 0 ).put( "TotalLobs",
                ( double ) csInfos.get( 0 ).get( "TotalLobs" )
                        + lobs * dstNodeNum );
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + lobs - 1 );
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * lobs );
        csInfos.get( 0 ).put( "TotalLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobSize" )
                        + lobSize * lobs * dstNodeNum );
        csInfos.get( 0 ).put( "TotalValidLobSize",
                ( double ) csInfos.get( 0 ).get( "TotalValidLobSize" )
                        + lobSize * lobs * dstNodeNum );
        csInfos.get( 0 ).put( "TotalLobPages",
                ( double ) csInfos.get( 0 ).get( "TotalLobPages" )
                        + lobPages * lobs * dstNodeNum );
        csInfos.get( 0 ).put( "TotalUsedLobSpace",
                ( double ) csInfos.get( 0 ).get( "TotalUsedLobSpace" )
                        + lobPages * lobPageSize * lobs
                                * ( srcNodeNum + dstNodeNum ) / 2 );
        csInfos.get( 0 ).put( "FreeLobSpace",
                ( double ) csInfos.get( 0 ).get( "LobCapacity" )
                        - ( double ) csInfos.get( 0 )
                                .get( "TotalUsedLobSpace" ) );
        csInfos.get( 0 ).put( "FreeLobSize",
                ( double ) csInfos.get( 0 ).get( "FreeLobSpace" ) );
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

                DBCollection dbcl1 = sdb.getCollectionSpace( csName )
                        .getCollection( clName1 );
                dbcl1.split( groupNames.get( 0 ), groupNames.get( 1 ), 100 );
            }
        }
    }

    private class PutLob extends ResultStore {

        @ExecuteOrder(step = 1)
        public void putLob()
                throws InterruptedException, NoSuchAlgorithmException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl2 = sdb.getCollectionSpace( csName )
                        .getCollection( clName2 );
                DBLob lob = dbcl2.createLob();
                lob.write( lobBuff );
                lob.close();
            }
        }
    }
}