package com.sequoiadb.split;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
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
 * @FileName:SEQDB-10531 切分过程中删除数据 :1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中删除数据（记录+lob），分别在如下几个阶段执行删除:
 *                       a、迁移数据过程中（删除数据包含迁移数据） b、清除数据过程中（删除数据包含清除数据）
 *                       c、切分任务结束后（删除数据覆盖源组和目标组） 4、查看切分和插入操作结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10531 extends SdbTestBase {
    private String clName = "testcaseCL10531";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    List< BSONObject > insertedData = new ArrayList< BSONObject >();// 所有已插入的数据
    List< ObjectId > insertedLob = new ArrayList< ObjectId >(); // 所有已插入的lobid
    List< ObjectId > deletedLob = new ArrayList< ObjectId >(); // 切分过程中被删除的Lobid

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

            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl );// 写入待切分的记录（1000普通记录，1000lob）
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
            for ( int i = 0; i < 1000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
                insertedData.add( obj );

                DBLob lob = cl.createLob();
                String id = lob.getID().toString();
                lob.write( id.getBytes() );
                lob.close();
                insertedLob.add( lob.getID() );
            }
            cl.bulkInsert( insertedData, 0 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }
    }

    // 切分删除数据，检查
    @Test()
    public void insertAndCheck() {
        Sequoiadb db = null;
        Split splitThread = new Split();
        DeleteDataAndLob deleteDataAndLob = new DeleteDataAndLob();
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            splitThread.start();
            deleteDataAndLob.start();
            // 等待切分线程
            if ( !splitThread.isSuccess() ) {
                Assert.fail( splitThread.getErrorMsg() );
            }
            deleteDataAndLob.join();
            if ( !deleteDataAndLob.isSuccess() ) {
                Assert.fail( deleteDataAndLob.getErrorMsg() );
            }
            // 目标组中的普通记录应当是insertedData的子集，校验完成后，将删除insertedData中属于子集的元素
            checkGroupData( insertedData, db, destGroupName );
            // 源中的普通记录应当是insertedData的子集，校验完成后，将删除insertedData中属于子集的元素
            checkGroupData( insertedData, db, srcGroupName );
            // 经过两次校验，insertedData应当为空
            Assert.assertEquals( insertedData.size() == 0, true,
                    "srcGroup and destGroup can not find:" + insertedData );

            // 校验源和目标组LOB记录
            checkGroupLob( insertedLob, db, destGroupName );
            checkGroupLob( insertedLob, db, srcGroupName );
            Assert.assertEquals( insertedLob.size() == 0, true,
                    "srcGroup and destGroup can not find:" + insertedLob );

            // 查询被删除的数据
            queryDeletedData( db );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private void queryDeletedData( Sequoiadb db ) {
        DBCursor cursor1 = null;
        try {
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection commCL = db.getCollectionSpace( csName )
                    .getCollection( clName );
            // 查询被删除的普通记录（sk<500）
            cursor1 = commCL.query( "{sk:{lt:500}}", null, null, null );
            if ( cursor1.hasNext() ) {
                Assert.fail( "delete record can be find:" + cursor1.getNext() );
            }

            // 查询被删除的Lob
            for ( int i = 0; i < deletedLob.size(); i++ ) {
                try {
                    DBLob tmp = commCL.openLob( deletedLob.get( i ) );
                    Assert.fail( tmp.getID().toString() );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -4 ) {
                        e.printStackTrace();
                        Assert.assertEquals( e.getErrorCode(), -4,
                                e.getMessage() + "\r\n"
                                        + SplitUtils.getKeyStack( e, this ) );
                    }
                }
            }

        } catch ( BaseException e ) {
            String stack = SplitUtils.getKeyStack( e, this );
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) + "\r\n" + stack );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
        }
    }

    private void checkGroupLob( List< ObjectId > insertedLob, Sequoiadb sdb,
            String destGroupName ) {
        Sequoiadb destDataNode = null;
        DBCursor cursor = null;
        try {
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得源主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );

            cursor = destCL.listLobs();
            int lobCount = 0;
            while ( cursor.hasNext() ) {
                ObjectId oid = ( ObjectId ) cursor.getNext().get( "Oid" );
                DBLob lob = destCL.openLob( oid );
                Assert.assertEquals( insertedLob.contains( lob.getID() ),
                        true );
                byte[] buffer = new byte[ 128 ];
                int length = lob.read( buffer );
                String content = new String( buffer, 0, length, "UTF-8" );
                Assert.assertEquals( lob.getID().toString().equals( content ),
                        true );
                lob.close();
                lobCount++;
                insertedLob.remove( lob.getID() );
            }
            // 数据量应在250条左右（总量1000-500，切分范围2048-4096）
            Assert.assertEquals(
                    lobCount > 250 - ( 250 * 0.3 )
                            && lobCount < 250 + ( 250 * 0.3 ),
                    true, "srcGroup count:" + lobCount );
        } catch ( BaseException | UnsupportedEncodingException e ) {
            StringBuffer stackBuffer = new StringBuffer();
            StackTraceElement[] stackElements = e.getStackTrace();
            for ( int i = 0; i < stackElements.length; i++ ) {
                if ( stackElements[ i ].toString()
                        .contains( this.getClass().getName() ) ) {
                    stackBuffer.append( stackElements[ i ].toString() )
                            .append( "\r\n" );
                }
            }
            Assert.fail(
                    e.getMessage() + "\r\n" + SplitUtils.getKeyStack( e, this )
                            + "\r\n" + stackBuffer );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
        }
    }

    private void checkGroupData( List< BSONObject > insertedData, Sequoiadb sdb,
            String groupName ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );

            cursor = cl.query( null, null, "{sk:1}", null );
            while ( cursor.hasNext() ) {
                BSONObject actual = cursor.getNext();
                Assert.assertEquals( insertedData.contains( actual ), true,
                        "insertedData can not find this record:" + actual );
                insertedData.remove( actual );
            }
            long count = cl.getCount();
            // 组的数据量应该在250条左右（总量1000-500，切分范围2048-4096）
            Assert.assertEquals(
                    count > 250 - ( 250 * 0.3 ) && count < 250 + ( 250 * 0.3 ),
                    true, "destGroup data count:" + count );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( dataNode != null ) {
                dataNode.disconnect();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace commCS = commSdb.getCollectionSpace( csName );

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

    class DeleteDataAndLob extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try {
                Sequoiadb db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 500; i++ ) { //// 删除500记录,500LOB
                    cl.removeLob( insertedLob.get( i ) );
                    cl.delete( insertedData.get( i ) );
                    deletedLob.add( insertedLob.get( i ) );
                    insertedData.remove( i );
                    insertedLob.remove( i );
                    Thread.sleep( 20 );
                }
            } catch ( BaseException e ) {
                Assert.fail( e.getMessage() + "\r\n"
                        + SplitUtils.getKeyStack( e, this ) );
            }
        }

    }

    class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName, 50 );
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
