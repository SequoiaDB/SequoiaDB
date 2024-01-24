package com.sequoiadb.split;

import java.util.ArrayList;
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
 * @FileName:SEQDB-10536 切分过程中删除id索引 seqDB-15551:删除id索引与切分并发 1、向cl中插入数据记录，创建ID索引
 *                       2、执行split，设置切分条件 3、切分过程中删除id索引（源组清除数据之前删除id索引）
 *                       4、查看切分和删除id索引结果
 * @author huangqiaohui
 * @version 3.00 *
 */

public class Split10536 extends SdbTestBase {
    private String clName = "testcaseCL_10536";
    private String srcGroup;
    private String desGroup;
    private Sequoiadb commSdb = null;

    @BeforeClass
    public void setUp() {
        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            if ( CommLib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            List< String > groupsName = CommLib.getDataGroupNames( commSdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            srcGroup = groupsName.get( 0 );
            desGroup = groupsName.get( 1 );

            CollectionSpace customCS = commSdb.getCollectionSpace( csName );
            DBCollection cl = customCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{sk:1},ShardingType:'range',Group:'"
                                    + srcGroup + "'}" ) );
            insertData( cl );// 写入待切分的记录（5000）
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.close();
            }
            e.printStackTrace();
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    public void insertData( DBCollection cl ) {
        List< BSONObject > tmp = new ArrayList< BSONObject >();
        for ( int i = 0; i < 5000; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
            tmp.add( obj );
        }
        cl.insert( tmp );
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        Split splitThread = null;
        DBCollection cl = null;
        try {
            // 启动切分线程
            splitThread = new Split();
            splitThread.start();

            // 删除id索引
            db = new Sequoiadb( coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            for ( int i = 0; i < 50; i++ ) {
                BasicBSONObject matcher = new BasicBSONObject();
                matcher.put( "Name", csName + "." + clName );
                matcher.put( "Status", new BasicBSONObject( "$ne", 9 ) );
                DBCursor cursor = db.listTasks( matcher, null, null, null );
                if ( cursor.hasNext() ) {
                    try {
                        cl.dropIdIndex();
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -334 ) {
                            throw e;
                        }
                    }
                    break;
                }
            }

            // 等待切分结束
            if ( !splitThread.isSuccess() ) {
                splitThread.getExceptions().get( 0 ).printStackTrace();
                Assert.fail( splitThread.getErrorMsg() );
            }

            // 查看索引
            checkIndex( cl );

            // 校验源组和目标组的数据 $gte:大于等于，$lt:小于
            checkData( db, 4500, "{sk:{$gte:500,$lt:5000}}", 4500, desGroup );
            checkData( db, 500, "{sk:{$gte:0,$lt:500}}", 500, srcGroup );

        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
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
                commSdb.close();
            }
        }
    }

    class Split extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            DBCollection cl = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                cl = sdb.getCollectionSpace( csName ).getCollection( clName );
                cl.split( srcGroup, desGroup, 90 );
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

    public void checkIndex( DBCollection cl ) {
        try {
            cl.getIndexInfo( "$id" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -47 ) {
                throw e;
            }
        }
    }

    private void checkData( Sequoiadb db, int expectedCount, String macher,
            int expectTotalCount, String group ) {
        Sequoiadb desDataNode = null;
        DBCollection desCL = null;
        try {
            desDataNode = db.getReplicaGroup( group ).getMaster().connect();
            desCL = desDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = desCL.getCount( macher );
            String hostandport = desDataNode.getHost() + ":"
                    + desDataNode.getPort();
            Assert.assertEquals( count, expectedCount, hostandport );// 目标组应当含有上述查询数据
            Assert.assertEquals( desCL.getCount(), expectTotalCount,
                    hostandport ); // 目标组应当含有的数据量
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( desDataNode != null ) {
                desDataNode.close();
            }
        }
    }
}