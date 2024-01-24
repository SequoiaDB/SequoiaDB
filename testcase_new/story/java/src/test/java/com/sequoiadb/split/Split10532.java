package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
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
 * @FileName:SEQDB-10532 切分过程中更新数据 1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中更新数据，同时插入数据，分别在如下几个阶段执行更新: a、迁移数据过程中（更新数据包含迁移数据）
 *                       b、清除数据过程中（更新数据包含清除数据） 更新数据同时满足如下条件： a、包含切分范围边界值数据
 *                       b、覆盖源组和目标组范围 c、包含源组中数据和切分过程中新插入数据 4、查看切分和更新操作结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10532 extends SdbTestBase {
    private String clName = "testcaseCL_10532";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    public static final int FLG_INSERT_CONTONDUP = 0x00000001;

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
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl );// 写入待切分的记录（1000）
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
            ArrayList< BSONObject > arr = new ArrayList< BSONObject >();
            for ( int i = 0; i < 100000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON
                        .parse( "{sk:" + i + ",alpha:1}" );
                arr.add( obj );
            }
            cl.bulkInsert( arr, this.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            throw e;
        }

    }

    @Test
    public void updateAndInsert() {
        Sequoiadb db = null;
        Sequoiadb destDataNode = null;
        Sequoiadb srcDataNode = null;
        Split splitThread = null;
        try {
            // 切分线程启动
            splitThread = new Split();
            // 增加和更新数据
            UpdateAndInsert updateAndInsertThread = new UpdateAndInsert();
            splitThread.start();
            updateAndInsertThread.start();
            db = new Sequoiadb( coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );

            // 等待切分结束，并检查出错信息
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
            Assert.assertEquals( updateAndInsertThread.isSuccess(), true,
                    updateAndInsertThread.getErrorMsg() );
            // 构造源组期望数据
            List< BSONObject > srcExpect = new ArrayList< BSONObject >();
            for ( int i = 0; i < 40000; i++ ) {
                srcExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:1}" ) );
            }
            for ( int i = 40000; i < 50000; i++ ) {
                srcExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:2}" ) );
                srcExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:2}" ) );
            }
            // 检验源组数据
            checkGroupData( db, srcGroupName, srcExpect );

            // 构造目标组期望数据
            List< BSONObject > destExpect = new ArrayList< BSONObject >();
            for ( int i = 50000; i < 60000; i++ ) {
                destExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:2}" ) );
                destExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:2}" ) );
            }
            for ( int i = 60000; i < 100000; i++ ) {
                destExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:1}" ) );
            }
            // 检验目标组数据
            checkGroupData( db, destGroupName, destExpect );

            // 构造更新后的期望数据
            List< BSONObject > updateExpect = new ArrayList< BSONObject >();
            for ( int i = 40000; i < 60000; i++ ) {
                updateExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:2}" ) );
                updateExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",alpha:2}" ) );
            }
            // 查询被更新的数据
            queryUpdatedData( db, updateExpect );

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

    @AfterClass()
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

    private void queryUpdatedData( Sequoiadb db, List< BSONObject > dataList ) {
        DBCursor cursor1 = null;
        try {
            // 比对所有更新的数据
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            cursor1 = cl.query( "{sk:{$gte:40000,$lt:60000}}", null, "", null );
            while ( cursor1.hasNext() ) {
                BSONObject actual = cursor1.getNext();
                actual.removeField( "_id" );
                Assert.assertEquals( dataList.contains( actual ), true,
                        "insertedData can not find this record:" + actual );
                dataList.remove( actual );
            }
            Assert.assertEquals( dataList.size(), 0,
                    "miss some records:" + dataList );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
        }

    }

    private void checkGroupData( Sequoiadb db, String groupName,
            List< BSONObject > expect ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        try {
            dataNode = db.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            cursor = cl.query( null, null, "{sk:1}", null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                obj.removeField( "_id" );
                actual.add( obj );
            }
            Assert.assertEquals( expect.equals( actual ), true,
                    "expect:" + expect + "\r\nactual:" + actual );
        } catch ( BaseException e ) {
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

    class UpdateAndInsert extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 40000; i < 60000; i++ ) {
                    cl.insert( "{sk:" + i + ",alpha:1}" );// 增加数据
                    cl.update( "{sk:" + i + ",alpha:1}", "{$inc:{alpha:1}}",
                            null );// 更新数据
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
                DBCollection subCL = cs.getCollection( clName );
                subCL.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:50000}" ),
                        ( BSONObject ) JSON.parse( "{sk:100000}" ) );
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
