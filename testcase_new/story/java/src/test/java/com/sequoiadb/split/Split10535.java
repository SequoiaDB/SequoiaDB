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
 * @FileName:SEQDB-10535 切分过程中删除索引 1、向cl中插入数据记录，创建多个索引 2、执行split，设置切分条件
 *                       3、切分过程中删除索引 4、查看切分和删除索引结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10535 extends SdbTestBase {
    private String clName = "testcaseCL_10535";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private List< BSONObject > insertedData = new ArrayList< BSONObject >();

    @BeforeClass()
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
            insertDataAndCreateIndex( cl );// 写入待切分的记录（1000）,创建一个普通索引
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    public void insertDataAndCreateIndex( DBCollection cl ) {
        try {
            for ( int i = 0; i < 1000; i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "sk", i );
                for ( int j = 1; j <= 55; j++ ) {
                    obj.put( "index" + j, i );
                }
                cl.insert( obj );
                insertedData.add( obj );
            }
            for ( int i = 1; i <= 55; i++ ) {
                cl.createIndex( "index" + i, "{index" + i + ":1}", false,
                        false );
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

    @Test()
    public void delteIndex() {
        Sequoiadb db = null;
        Sequoiadb destDataNode = null;
        Sequoiadb srcDataNode = null;
        Split splitThread = null;
        try {
            // 启动切分线程
            splitThread = new Split();
            DeleteIndex deleteIndex = new DeleteIndex();
            splitThread.start();
            deleteIndex.start();
            db = new Sequoiadb( coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            // 等待切分结束
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
            Assert.assertEquals( deleteIndex.isSuccess(), true,
                    splitThread.getErrorMsg() );
            // 期望有500条符合{sk:{$gte:0,$lt:500}}的记录，并且源组中只有500条记录
            checkGroupData( db, 500, "{sk:{$gte:0,$lt:500}}", 500,
                    srcGroupName );
            // 期望有500条符合条件的记录，并且目标组中只有500条记录
            checkGroupData( db, 500, "{sk:{$gte:500,$lt:1000}}", 500,
                    destGroupName );

            // 分别在源，目标，coord检查索引情况(均不存在普通索引)
            destDataNode = db.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            srcDataNode = db.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();// 获得源组主节点链接
            DBCollection srcCL = srcDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            checkIndexNonExist( destCL );
            checkIndexNonExist( srcCL );
            checkIndexNonExist( cl );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
            if ( srcDataNode != null ) {
                srcDataNode.disconnect();
            }
            if ( destDataNode != null ) {
                destDataNode.disconnect();
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

    // 检查CL是否仅存在默认生成的id，shard索引
    public void checkIndexNonExist( DBCollection cl ) {
        DBCursor dbc = null;
        BSONObject idIndex = ( BSONObject ) JSON
                .parse( "{name: \"$id\",key: {_id: 1}}" );
        BSONObject shardingKeyIndex = ( BSONObject ) JSON
                .parse( "{name: \"$shard\",key: {sk: 1}}" );
        ArrayList< BSONObject > expect = new ArrayList< BSONObject >();
        expect.add( idIndex );
        expect.add( shardingKeyIndex );
        try {
            dbc = cl.getIndexes();
            while ( dbc.hasNext() ) {
                BSONObject record = ( BSONObject ) dbc.getNext()
                        .get( "IndexDef" );
                BasicBSONObject actual = new BasicBSONObject();
                actual.put( "name", record.get( "name" ) );
                actual.put( "key", record.get( "key" ) );
                if ( expect.contains( actual ) ) {
                    expect.remove( actual );
                } else {
                    Assert.fail( "should not have this index:" + actual );
                }
            }
            Assert.assertEquals( expect.size() == 0, true,
                    "miss some indexes:" + expect );
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    private void checkGroupData( Sequoiadb db, int expectedCount, String macher,
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
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
        }
    }

    class DeleteIndex extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 1; i <= 55; i++ ) {
                    cl.dropIndex( "index" + i );
                    Thread.sleep( 100 );
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
                        ( BSONObject ) JSON.parse( "{sk:500}" ), // 切分
                        ( BSONObject ) JSON.parse( "{sk:1000}" ) );
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
