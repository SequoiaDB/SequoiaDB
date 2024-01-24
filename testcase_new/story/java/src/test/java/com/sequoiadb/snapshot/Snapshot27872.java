package com.sequoiadb.snapshot;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * @version 1.10
 * @Description seqDB-27872:WRITE和SHARED_READ|WRITE并发读写lob后查看快照
 * @Author huanghaimei
 * @Date 2022.11.03
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.02.08
 */
public class Snapshot27872 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private String csName = "cs_27872";
    private String clName = "cl27872";
    private int lobSize = 1024 * 100;
    private int lobPageSize = 262144;
    private byte[] expData = new byte[ lobSize ];

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        dbcs = sdb.createCollectionSpace( csName,
                new BasicBSONObject( "LobPageSize", lobPageSize ) );
        BasicBSONObject option = new BasicBSONObject();
        option.append( "ReplSize", -1 );
        option.append( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        dbcl = dbcs.createCollection( clName, option );
    }

    @Test
    public void testLob() {
        byte[] writeLobBuff = RandomWriteLobUtil.tenKbuff;
        int writeSize = writeLobBuff.length;

        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();
        db3 = CommLib.getRandomSequoiadb();

        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        byte[] lobBuff = RandomWriteLobUtil.lobBuff;
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl3 = db3.getCollectionSpace( csName )
                .getCollection( clName );

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

        byte[] readLobBuff = new byte[ writeSize ];
        DBLob lob1 = cl1.openLob( id, DBLob.SDB_LOB_WRITE );
        DBLob lob2 = cl2.openLob( id,
                DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
        DBLob lob3 = cl3.openLob( id,
                DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );

        lob1.lockAndSeek( writeSize, writeSize );
        lob1.write( writeLobBuff );
        expData = RandomWriteLobUtil.appendBuff( lobBuff, writeLobBuff,
                writeSize );

        lob2.lockAndSeek( writeSize * 2 + 1024, writeSize );
        lob2.read( readLobBuff );

        try {
            byte[] readData = new byte[ writeSize ];
            lob3.lockAndSeek( 0, writeSize + 1024 );
            lob3.read( readData );
            Assert.fail( "there should be a lock conflict here." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_LOB_LOCK_CONFLICTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        lob3.lockAndSeek( writeSize * 4, writeSize );
        lob3.write( writeLobBuff );
        expData = RandomWriteLobUtil.appendBuff( expData, writeLobBuff,
                writeSize * 4 );

        try {
            lob2.lockAndSeek( 1024, writeSize + 1024 );
            lob2.write( writeLobBuff );
            Assert.fail( "there should be a lock conflict here." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_LOB_LOCK_CONFLICTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        lob1.close();
        lob2.close();
        lob3.close();

        byte[] expData1 = Arrays.copyOfRange( lobBuff, writeSize * 2 + 1024,
                writeSize * 3 + 1024 );
        RandomWriteLobUtil.assertByteArrayEqual( readLobBuff, expData1,
                "lob data is wrong" );
        // 校验数据库快照
        int loadMonitorNodeNum = 2;
        int lobPages = ( lobSize + 1023 ) / lobPageSize + 1;
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" )
                        + loadMonitorNodeNum * 3 );
        dataInfos.get( 0 ).put( "TotalLobGet",
                ( double ) dataInfos.get( 0 ).get( "TotalLobGet" ) + 4 );
        dataInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobSize * loadMonitorNodeNum * 2 );
        dataInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * loadMonitorNodeNum * 3 );
        dataInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobPageSize + 4 );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPageSize + 4 );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" ) + 7 );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages );

        // 校验集合空间快照
        csInfos.get( 0 ).put( "TotalLobGet",
                ( double ) csInfos.get( 0 ).get( "TotalLobGet" ) + 2 );
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + 3 );
        csInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobPageSize );
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize );
        csInfos.get( 0 ).put( "TotalLobRead",
                ( double ) csInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPageSize * 2 );
        csInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) csInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobPageSize * 2 );
        csInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) csInfos.get( 0 ).get( "TotalLobAddressing" ) + 7 );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject( "Name", csName ), null, null );
        SnapshotUtil.checkSnapshot( cursor, csInfos, lobPages );
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
            if ( db1 != null ) {
                db1.close();
            }
            if ( db2 != null ) {
                db2.close();
            }
            if ( db3 != null ) {
                db3.close();
            }
        }
    }
}