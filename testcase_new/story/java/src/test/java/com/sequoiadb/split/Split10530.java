package com.sequoiadb.split;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingDeque;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10530 切分过程中插入数据 :1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中插入数据（记录+lob），持续插入数据覆盖如下阶段: a、迁移数据过程中（插入数据包含迁移数据）
 *                       b、清除数据过程中（插入数据包含清除数据） 插入数据同时满足如下条件： a、包含切分范围边界值数据
 *                       b、覆盖源组和目标组范围 4、查看切分和插入操作结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10530 extends SdbTestBase {
    private String clName = "testcaseCL_10530";
    private CollectionSpace commCS;
    private DBCollection cl;
    List< String > groupsName = new ArrayList<>();
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private ConcurrentHashMap< ObjectId, String > id2md5 = new ConcurrentHashMap< ObjectId, String >();
    private LinkedBlockingDeque< ObjectId > oidQueue = new LinkedBlockingDeque< ObjectId >();
    private Random random = new Random();
    List< BSONObject > insertedData = new ArrayList< BSONObject >();// 所有已插入的数据
    List< ObjectId > insertedLob = new ArrayList< ObjectId >(); // 所有已插入的lobid

    @BeforeClass()
    public void setUp() {
        commSdb = new Sequoiadb( coordUrl, "", "" );
        commSdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{'PreferedInstance':'M'}" ) );
        // 跳过 standAlone 和数据组不足的环境
        CommLib commlib = new CommLib();
        if ( commlib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        groupsName = commlib.getDataGroupNames( commSdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }

        commCS = commSdb.getCollectionSpace( SdbTestBase.csName );
        cl = commCS.createCollection( clName, ( BSONObject ) JSON.parse(
                "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                        + groupsName.get( 0 ) + "'}" ) );
        // 写入待切分的记录（0-10000)
        insertData( cl, 0, 10000 );
    }

    // 切分同时插入数据，检查
    @Test
    public void insertAndCheck() {
        Split splitThread = new Split();
        splitThread.start();

        PutLobsTask putLobsTask = new PutLobsTask();
        putLobsTask.start();

        int beginNo = 10000;
        int endNo = 40000;
        insertData( cl, beginNo, endNo );

        Assert.assertTrue( splitThread.isSuccess(), splitThread.getErrorMsg() );
        Assert.assertTrue( putLobsTask.isSuccess(), putLobsTask.getErrorMsg() );

        // check the result
        checkDataSplitResult();
        // 从coord上查询记录正确
        checkRecordFromCoord();
        checkLobData();

    }

    @AfterClass()
    public void tearDown() {
        try {
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    private void checkRecordFromCoord() {
        DBCursor cursor1 = null;
        try {
            // 将insertedData中的每一条数据作为macher(覆盖查询边界)
            for ( int i = 0; i < insertedData.size(); i++ ) {
                cursor1 = cl.query( insertedData.get( i ), null, null, null,
                        DBQuery.FLG_QUERY_WITH_RETURNDATA );
                BSONObject expect = insertedData.get( i );
                if ( cursor1.hasNext() ) {
                    BSONObject actual = cursor1.getNext();
                    Assert.assertEquals( expect.equals( actual ), true,
                            "expect:" + expect + " actual:" + actual );
                    if ( cursor1.hasNext() ) {
                        Assert.fail(
                                "query more than tow record mach expetedData:"
                                        + cursor1.getNext() );
                    }
                } else {
                    Assert.fail( "query can not find:" + expect );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
        }
    }

    private void checkDataSplitResult() {
        long allCount = 0;
        long allLobs = 0;
        for ( int i = 0; i < 2; i++ ) {
            Sequoiadb dataNode = null;
            try {
                dataNode = commSdb.getReplicaGroup( groupsName.get( i ) )
                        .getMaster().connect();// 获得目标组主节点链接
                DBCollection cl = dataNode
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                long count = cl.getCount();
                allCount += count;
                // 组的数据量应该在20000条左右（总量40000，切分范围2048-4096）
                double actRecordsErrorValue = Math.abs( 40000 / 2 - count )
                        / 40000 / 2;
                if ( actRecordsErrorValue > 0.5 ) {
                    Assert.assertTrue( false, "errorValue: "
                            + actRecordsErrorValue + " subCont:" + count );
                }

                long lobcount = 0;
                try ( DBCursor listLob = cl.listLobs()) {
                    while ( listLob.hasNext() ) {
                        listLob.getNext();
                        lobcount++;
                    }
                }
                allLobs += lobcount;
                // listlobs数量应该在200个左右（总lob数400，切分比例50%）
                double actErrorValue = Math.abs( 400 / 2 - lobcount ) / 400 / 2;
                if ( actErrorValue > 0.5 ) {
                    Assert.assertTrue( false, "errorValue: " + actErrorValue
                            + " subCont:" + lobcount );
                }
            } finally {
                if ( dataNode != null ) {
                    dataNode.disconnect();
                }
            }
        }

        // 获取的记录总数应该和插入的记录数相同,listlobs总数应该和插入的lob数相同
        Assert.assertEquals( allCount, insertedData.size() );
        Assert.assertEquals( allLobs, oidQueue.size() );
    }

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int lobtimes = 400;
                writeLobAndGetMd5( dbcl, lobtimes );
            }
        }
    }

    private void writeLobAndGetMd5( DBCollection cl, int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 512 );
            ;
            byte[] wlobBuff = getRandomBytes( writeLobSize );
            DBLob wLob = cl.createLob();
            wLob.write( wlobBuff );
            ObjectId oid = wLob.getID();
            wLob.close();
            // save oid and md5
            String prevMd5 = getMd5( wlobBuff );
            oidQueue.offer( oid );
            id2md5.put( oid, prevMd5 );
        }
    }

    static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    class Split extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            srcGroupName = groupsName.get( 0 );
            destGroupName = groupsName.get( 1 );
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName, 50 );
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

    private void insertData( DBCollection cl, int beginNo, int endNo ) {
        int count = 0;
        List< BSONObject > list = new ArrayList< BSONObject >();
        for ( int i = beginNo; i < endNo; i++ ) {
            count++;
            BSONObject obj = ( BSONObject ) JSON.parse(
                    "{sk:" + i + ", test:" + "'testasetatatatatat'" + "}" );
            list.add( obj );
            insertedData.add( obj );
            if ( count % 10000 == 0 ) {
                cl.insert( list );
                list.clear();
            }
        }
    }

    private void checkLobData() {
        int count = 0;
        try ( DBCursor listLob = cl.listLobs()) {
            while ( listLob.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) listLob.getNext();
                ObjectId existOid = obj.getObjectId( "Oid" );
                try ( DBLob rLob = cl.openLob( existOid )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = LobOprUtils.getMd5( rbuff );
                    String prevMd5 = id2md5.get( existOid );
                    Assert.assertEquals( curMd5, prevMd5 );
                }
                count++;
            }
        }
        // the list lobnums must be consistent with the number of remaining
        // digits in the actual map:id2md5
        Assert.assertEquals( count, id2md5.size() );
    }

    public static String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
    }

}
