package com.sequoiadb.snapshot;

import java.util.ArrayList;
import java.util.Arrays;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @version 1.10
 * @Description seqDB-27874:READ和SHARED_READ|WRITE并发读写lob后查看快照
 * @Author huanghaimei
 * @Date 2022.11.03
 * @UpdateAuthor huanghaimei
 * @UpdateDate 2023.02.08
 */
public class Snapshot27874 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private String csName = "cs_27874";
    private String clName = "cl27874";
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

        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
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

        byte[] readLob1 = new byte[ writeSize ];
        DBLob lob1 = cl1.openLob( id, DBLob.SDB_LOB_READ );
        lob1.lockAndSeek( writeSize, writeSize );
        lob1.read( readLob1 );
        byte[] expData1 = Arrays.copyOfRange( lobBuff, writeSize,
                writeSize * 2 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob1, expData1,
                "lob data is wrong" );

        try {
            DBLob lob2 = cl2.openLob( id,
                    DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
            lob2.close();
            Assert.fail(
                    "can't open other mode when the lob open with read mode." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_LOB_IS_IN_USE
                    .getErrorCode() ) {
                throw e;
            }
        }
        lob1.close();

        lob1 = cl1.openLob( id, DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
        lob1.lockAndSeek( writeSize, writeSize );
        lob1.read( readLob1 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob1, expData1,
                "lob data is wrong" );
        try {
            DBLob lob2 = cl2.openLob( id, DBLob.SDB_LOB_READ );
            lob2.close();
            Assert.fail(
                    "can't open read mode when the lob open with other mode." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_LOB_IS_IN_USE
                    .getErrorCode() ) {
                throw e;
            }
        }
        lob1.close();

        lob1 = cl1.openLob( id, DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
        lob1.lockAndSeek( writeSize, writeSize );
        lob1.write( writeLobBuff );
        expData = RandomWriteLobUtil.appendBuff( lobBuff, writeLobBuff,
                writeSize );
        try {
            DBLob lob2 = cl2.openLob( id, DBLob.SDB_LOB_READ );
            lob2.close();
            Assert.fail(
                    "can't open read mode when the lob open with other mode." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_LOB_IS_IN_USE
                    .getErrorCode() ) {
                throw e;
            }
        }
        lob1.close();

        // 校验数据库快照
        int loadMonitorNodeNum = 2;
        int lobPages = ( lobSize + 1023 ) / lobPageSize + 1;
        dataInfos.get( 0 ).put( "TotalLobGet",
                ( double ) dataInfos.get( 0 ).get( "TotalLobGet" )
                        + loadMonitorNodeNum * 3 );
        dataInfos.get( 0 ).put( "TotalLobPut",
                ( double ) dataInfos.get( 0 ).get( "TotalLobPut" ) + 4 );
        dataInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobSize * loadMonitorNodeNum * 4 );
        dataInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize * loadMonitorNodeNum * 3 );
        dataInfos.get( 0 ).put( "TotalLobRead",
                ( double ) dataInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPageSize + 4 );
        dataInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) dataInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobPageSize + 4 );
        dataInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) dataInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages + 6 );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                new BasicBSONObject(), null, null );
        SnapshotUtil.checkSnapshot( cursor, dataInfos, lobPages );

        // 校验集合空间快照
        csInfos.get( 0 ).put( "TotalLobGet",
                ( double ) csInfos.get( 0 ).get( "TotalLobGet" ) + 3 );
        csInfos.get( 0 ).put( "TotalLobPut",
                ( double ) csInfos.get( 0 ).get( "TotalLobPut" ) + 2 );
        csInfos.get( 0 ).put( "TotalLobReadSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobReadSize" )
                        + lobPageSize );
        csInfos.get( 0 ).put( "TotalLobWriteSize",
                ( double ) csInfos.get( 0 ).get( "TotalLobWriteSize" )
                        + lobSize );
        csInfos.get( 0 ).put( "TotalLobRead",
                ( double ) csInfos.get( 0 ).get( "TotalLobRead" )
                        + lobPageSize );
        csInfos.get( 0 ).put( "TotalLobWrite",
                ( double ) csInfos.get( 0 ).get( "TotalLobWrite" )
                        + lobPageSize );
        csInfos.get( 0 ).put( "TotalLobAddressing",
                ( double ) csInfos.get( 0 ).get( "TotalLobAddressing" )
                        + lobPages + 6 );
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
        }
    }
}
