package com.sequoiadb.split;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingDeque;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-512 数据切分过程中插入lob对象 1.在cl下指定分区键进行数据切分
 *                     2、切分过程中向cl中插入大量lob对象，如插入1百万条记录 3、查看数据切分结果
 *                     4、再次插入数据，查看写数据情况
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split512 extends SdbTestBase {
    private String clName = "testcaseCL512";
    private CollectionSpace commCS;
    private DBCollection cl;
    private ArrayList< String > groupNames = new ArrayList<>();
    Sequoiadb commSdb = null;
    private ConcurrentHashMap< ObjectId, String > id2md5 = new ConcurrentHashMap<>();
    private LinkedBlockingDeque< ObjectId > oidQueue = new LinkedBlockingDeque<>();
    private Random random = new Random();

    @BeforeClass
    public void setUp() {
        commSdb = new Sequoiadb( coordUrl, "", "" );
        commSdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{PreferedInstance: 'M'}" ) );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        if ( CommLib.getDataGroupNames( commSdb ).size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }

        commCS = commSdb.getCollectionSpace( csName );
        cl = commCS.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"a\":1},ShardingType:\"hash\"}" ) );
        groupNames = SplitUtils.getGroupName( commSdb, csName, clName ); // 获取切分组名
        int lobtimes = 100;
        writeLobAndGetMd5( cl, lobtimes );
    }

    // 切分时，插入lob,等待切分完成,检查目标组数据量，重新插入数据，检查落入情况
    @Test
    public void insertLob() {

        Split splitThread = new Split();
        splitThread.start();

        PutLobsTask putLobsTask = new PutLobsTask();
        putLobsTask.start();

        Assert.assertTrue( splitThread.isSuccess(), splitThread.getErrorMsg() );
        Assert.assertTrue( putLobsTask.isSuccess(), putLobsTask.getErrorMsg() );

        checkCatalog();

        // 切分完成后，再次插入lob
        int lobtimes = 100;
        writeLobAndGetMd5( cl, lobtimes );
        // 检查切分结果，插入lob结果
        checkDataSplitResult();
        checkLobData();
    }

    @SuppressWarnings("deprecation")
    @AfterClass
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

    // 检查编目信息的切分范围是否正确
    private void checkCatalog() {
        DBCursor dbc = null;
        try {
            dbc = commSdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            } else {
                Assert.fail( clName + " collection catalog not found" );
            }
            BSONObject expectLowBound = ( BSONObject ) JSON
                    .parse( "{\"\":2048}" );
            BSONObject expectUpBound = ( BSONObject ) JSON
                    .parse( "{\"\":4096}" );
            for ( int i = 0; i < list.size(); i++ ) {
                String groupName = ( String ) ( ( BSONObject ) list.get( i ) )
                        .get( "GroupName" );
                if ( groupName.equals( groupNames.get( 1 ) ) ) {
                    BSONObject actualLowBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "LowBound" );
                    BSONObject actualUpBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "UpBound" );
                    if ( actualLowBound.equals( expectLowBound )
                            && actualUpBound.equals( expectUpBound ) ) {
                        break;
                    } else {
                        Assert.fail( "check catalog fail" );
                    }
                }
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    class Split extends SdbThreadBase {
        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int lobtimes = 300;

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

    private String getMd5( Object inbuff ) {
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

    @SuppressWarnings("deprecation")
    private void checkDataSplitResult() {
        long allLobs = 0;
        for ( int i = 0; i < 2; i++ ) {
            Sequoiadb dataNode = null;
            try {
                dataNode = commSdb.getReplicaGroup( groupNames.get( i ) )
                        .getMaster().connect();// 获得目标组主节点链接
                DBCollection cl = dataNode
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                long lobcount = 0;
                try ( DBCursor listLob = cl.listLobs()) {
                    while ( listLob.hasNext() ) {
                        listLob.getNext();
                        lobcount++;
                    }
                }
                allLobs += lobcount;
                // listlobs数量应该在500个左右（总lob数500，切分比例50%）
                double actErrorValue = Math.abs( 500 / 2 - lobcount ) / 500 / 2;
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

        // listlobs总数应该和插入的lob数相同
        Assert.assertEquals( allLobs, oidQueue.size() );
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
                    String curMd5 = getMd5( rbuff );
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
}
