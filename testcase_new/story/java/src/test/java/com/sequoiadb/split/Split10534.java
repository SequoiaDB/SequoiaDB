package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10534 切分过程中创建索引 1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中创建索引，分别验证如下几个场景： a、迁移数据过程中 b、清除数据过程中 创建索引覆盖如下条件：
 *                       a、索引最大数（64个索引） b、索引包含正序、逆序 4、查看切分和创建索引结果
 *                       5、带索引查询切分数据（覆盖查询切分范围边界值数据） 此用例加入测试，问题单：2232(丢失索引问题已修复)
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10534 extends SdbTestBase {
    private String clName = "testcaseCL10534";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private List< BSONObject > insertedData = new ArrayList< BSONObject >();
    private List< BSONObject > indexes = new ArrayList< BSONObject >();
    public static final int FLG_INSERT_CONTONDUP = 0x00000001;

    @BeforeClass
    public void setUp() {

        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            List< String > groupsName = commlib.getDataGroupNames( commSdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            srcGroupName = groupsName.get( 0 );
            destGroupName = groupsName.get( 1 );

            CollectionSpace customCS = commSdb.getCollectionSpace( csName );
            DBCollection cl = customCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl );// 写入待切分的记录（500）
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    public void insertData( DBCollection cl ) {
        try {
            for ( int i = 0; i < 50000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON
                        .parse( "{sk:" + i + ",index1:" + i + "}" );
                // cl.insert(obj);
                insertedData.add( obj );
            }
            cl.bulkInsert( insertedData, this.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    @Test
    public void testCreateIndex() {
        Sequoiadb db = null;
        Split splitThread = null;
        try {
            // 启动切分线程
            splitThread = new Split();
            CreateIndex createIndexThread = new CreateIndex();
            splitThread.start();
            createIndexThread.start();

            // 建立62个索引，预期索引集合indexes
            db = new Sequoiadb( coordUrl, "", "" );
            // 预期索引集合indexes加入默认的id索引和shard索引
            BSONObject idIndex = ( BSONObject ) JSON
                    .parse( "{name: \"$id\",key: {_id: 1}}" );
            BSONObject shardingKeyIndex = ( BSONObject ) JSON
                    .parse( "{name: \"$shard\",key: {sk: 1}}" );
            indexes.add( idIndex );
            indexes.add( shardingKeyIndex );

            // 等待切分结束
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
            Assert.assertEquals( createIndexThread.isSuccess(), true,
                    createIndexThread.getErrorMsg() );
            // 期望有2500条符合{sk:{$gte:0,$lt:2500}}的记录，并且源组中只有250条记录
            checkDestGroup( db, 25000, "{sk:{$gte:0,$lt:25000}}", 25000,
                    srcGroupName );
            // 期望有2500条符合条件的记录，并且目标组中只有250条记录
            checkDestGroup( db, 25000, "{sk:{$gte:25000,$lt:50000}}", 25000,
                    destGroupName );

            // 检查源和目标组的索引
            checkIndexExist( db, srcGroupName );
            checkIndexExist( db, destGroupName );

            // 指定索引信息查询数据（在cl中匹配{index1:34}，期望结果{sk:34,index1:34}，且为ixscan）
            queryByIndexAndCheckExplain( db, "{index1:34}", "{sk:34,index1:34}",
                    "ixscan" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
            if ( splitThread != null ) {
                splitThread.join();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = commSdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    // 按macher查询，结果与expectedRecord比对，检查访问计划的扫描方式是否为expectScanType
    private void queryByIndexAndCheckExplain( Sequoiadb db, String macher,
            String expectedRecord, String expectScanType ) {
        DBCursor dbc1 = null;
        DBCursor dbc2 = null;
        BSONObject expected = ( BSONObject ) JSON.parse( expectedRecord );
        try {
            // 查询，检查结果的正确性
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            dbc1 = cl.query( macher, null, null, null );
            ArrayList< BSONObject > queryReaults = new ArrayList< BSONObject >();
            while ( dbc1.hasNext() ) {
                queryReaults.add( dbc1.getNext() );
            }
            if ( queryReaults.size() == 1 ) {
                BSONObject expect = ( BSONObject ) queryReaults.get( 0 );
                expect.removeField( "_id" );
                Assert.assertEquals( expected.equals( expect ), true,
                        "expected:" + expected.toString() + " actual:"
                                + expect );
            } else {
                Assert.fail(
                        "query resault not correct,the array:" + queryReaults );
            }

            // 检查扫描方式
            dbc2 = cl.explain( ( BSONObject ) JSON.parse( macher ), null, null,
                    null, 0, -1, 0, null );
            if ( dbc2.hasNext() ) {
                String scanType = ( String ) dbc2.getNext().get( "ScanType" );
                Assert.assertEquals( scanType.equals( expectScanType ), true,
                        "scanType is " + scanType + " not" + expectScanType );
            } else {
                Assert.fail( "mainCL explain wrong" );
            }
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc1 != null ) {
                dbc1.close();
            }
            if ( dbc2 != null ) {
                dbc2.close();
            }
        }
    }

    // 检查是否存在test方法中设置的索引
    public void checkIndexExist( Sequoiadb db, String groupName ) {
        DBCursor dbc = null;
        List< BSONObject > indexesCopy = new ArrayList< BSONObject >( indexes );
        Sequoiadb dataNode = null;
        try {
            dataNode = db.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            dbc = cl.getIndexes();
            while ( dbc.hasNext() ) {
                BSONObject record = ( BSONObject ) dbc.getNext()
                        .get( "IndexDef" );
                BasicBSONObject actual = new BasicBSONObject();
                actual.put( "name", record.get( "name" ) );
                actual.put( "key", record.get( "key" ) );
                if ( indexesCopy.contains( actual ) ) {
                    indexesCopy.remove( actual );
                } else {
                    Assert.fail( "should not have this index:" + actual );
                }
            }
            Assert.assertEquals( indexesCopy.size() == 0, true,
                    "miss some indexes:" + indexesCopy );
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
            if ( dataNode != null ) {
                dataNode.disconnect();
            }
        }
    }

    private void checkDestGroup( Sequoiadb db, int expectedCount, String macher,
            int expectTotalCount, String groupName ) {
        Sequoiadb destDataNode = null;
        try {
            destDataNode = db.getReplicaGroup( groupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = destCL.getCount( macher );
            Assert.assertEquals( count, expectedCount );// 目标组应当含有上述查询数据
            Assert.assertEquals( destCL.getCount(), expectTotalCount ); // 目标组应当含有的数据量
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
        }
    }

    class CreateIndex extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 62; i++ ) {
                    String indexName = "index" + i;
                    try {
                        Thread.sleep( 20 );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                    int oder = i % 2 == 0 ? 1 : -1;
                    cl.createIndex( indexName,
                            "{" + indexName + ":" + oder + "}", false, false );
                    BSONObject indexObj = ( BSONObject ) JSON
                            .parse( "{name:'" + indexName + "',key: {"
                                    + indexName + ": " + oder + "}}" );
                    indexes.add( indexObj );
                }
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }

    }

    class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:25000}" ), // 切分
                        ( BSONObject ) JSON.parse( "{sk:50000}" ) );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

}
