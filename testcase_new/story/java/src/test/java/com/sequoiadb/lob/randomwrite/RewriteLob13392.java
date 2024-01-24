package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName seqDB-13392 : truncate数据包含lob空切片
 * @Author linsuqiang
 * @Date 2017-11-16
 * @Version 1.00
 */

/*
 * 1、指定oid执行truncateLob操作，删除超过指定长度部分的数据，其中删除数据范围包含空切片，分别覆盖如下场景： a、truncate空切片
 * b、truncate数据包含空切片 2、检查操作结果（读取lob，查看lob对象长度，执行listLobs查看lobsize信息）
 */

public class RewriteLob13392 extends SdbTestBase {

    private final String csName = "lobTruncate13392";
    private final String clName = "lobTruncate13392";
    private final int lobPageSize = 128 * 1024; // 128k
    private final int lobMetaSize = 1 * 1024; // 1k

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        cl = cs.createCollection( clName, clOpt );
    }

    // a、truncate空切片
    @Test
    public void testLob() {
        int lobSize = 1;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        byte[] expData = data;

        // make the first data piece empty
        long firstDataPagePos = lobPageSize - lobMetaSize;
        long secondDataPagePos = firstDataPagePos + lobPageSize;
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.seek( secondDataPagePos, DBLob.SDB_LOB_SEEK_SET );
            byte[] data2 = new byte[ lobPageSize ];
            lob.write( data2 );
            expData = RandomWriteLobUtil.appendBuff( expData, data2,
                    lobPageSize );
        }

        // let empty piece be the last piece
        cl.truncateLob( oid, secondDataPagePos );
        expData = Arrays.copyOfRange( expData, 0, ( int ) secondDataPagePos );
        checkLobDataAndSize( cl, oid, expData, secondDataPagePos );

        // only truncate empty piece
        cl.truncateLob( oid, firstDataPagePos );
        expData = Arrays.copyOfRange( expData, 0, ( int ) firstDataPagePos );
        checkLobDataAndSize( cl, oid, expData, firstDataPagePos );

    }

    // b、truncate数据包含空切片
    @Test
    public void testLob2() {
        int lobSize = 1;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        byte[] expData = data;

        // make the first data piece empty
        long firstDataPagePos = lobPageSize - lobMetaSize;
        long secondDataPagePos = firstDataPagePos + lobPageSize;
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.seek( secondDataPagePos, DBLob.SDB_LOB_SEEK_SET );
            byte[] data2 = new byte[ lobPageSize ];
            lob.write( data2 );
            expData = RandomWriteLobUtil.appendBuff( expData, data2,
                    lobPageSize );
        }

        // truncate data that contains empty piece
        cl.truncateLob( oid, firstDataPagePos );
        expData = Arrays.copyOfRange( expData, 0, ( int ) firstDataPagePos );
        checkLobDataAndSize( cl, oid, expData, firstDataPagePos );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private void checkLobDataAndSize( DBCollection cl, ObjectId oid,
            byte[] expData, long expLen ) {
        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );

        long actSize = getSizeByListLobs( cl, oid );
        Assert.assertEquals( actSize, expLen,
                "wrong length after truncate lob" );
    }

    private long getSizeByListLobs( DBCollection cl, ObjectId oid ) {
        DBCursor cursor = null;
        long lobSize = 0;
        boolean oidFound = false;
        try {
            cursor = cl.listLobs();
            while ( cursor.hasNext() ) {
                BSONObject res = cursor.getNext();
                ObjectId curOid = ( ObjectId ) res.get( "Oid" );
                if ( curOid.equals( oid ) ) {
                    lobSize = ( long ) res.get( "Size" );
                    oidFound = true;
                    break;
                }
            }
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
        if ( !oidFound ) {
            throw new RuntimeException( "no such oid" );
        }
        return lobSize;
    }
}
