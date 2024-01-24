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
 * @FileName:SEQDB-10539 子表切分过程中主表执行插入/更新/删除操作 :1、向主表中插入数据记录
 *                       2、其中一个子表执行split，设置切分条件
 *                       3、切分过程中主表执行插入/更新/删除数据，分别在如下几个阶段执行操作:
 *                       a、迁移数据过程中（插入/更新/删除数据包含迁移数据） b、清除数据过程中（插入/更新/删除数据包含清除数据）
 *                       操作数据同时满足如下条件： a、包含切分范围边界值数据 b、覆盖源组和目标组范围
 *                       c、包含源组中数据和切分过程中新插入数据
 *                       4、查看切分和主表数据操作结果（主表查询数据覆盖切分范围边界值、子表挂载范围边界值）
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10539 extends SdbTestBase {
    private String subCLName = "testcaseSubCL_10539";
    private String mainCLName = "testcaseMainCL_10539";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;

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
            DBCollection mainCL = commCS.createCollection( mainCLName,
                    ( BSONObject ) JSON
                            .parse( "{ShardingKey:{sk:1},IsMainCL:true}" ) );
            DBCollection subCL = commCS.createCollection( subCLName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            mainCL.attachCollection( subCL.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk:0},UpBound:{sk:1000000}}" ) );
            insertData( mainCL );// 写入待切分的记录（1000）
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
            List< BSONObject > insertedData = new ArrayList< BSONObject >();
            for ( int i = 0; i < 100000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON
                        .parse( "{sk:" + i + ",alpha:" + i + "}" );
                insertedData.add( obj );
                // cl.insert(obj);
            }
            cl.bulkInsert( insertedData, 0 );
        } catch ( BaseException e ) {
            throw e;
        }

    }

    @Test
    public void testCrud() {
        Sequoiadb db = null;
        Sequoiadb destDataNode = null;
        Sequoiadb srcDataNode = null;
        Split splitThread = null;
        Crud crudThread = null;
        try {
            // 启动切分线程
            splitThread = new Split();
            crudThread = new Crud();
            splitThread.start();
            crudThread.start();

            // 增删改
            db = new Sequoiadb( coordUrl, "", "" );
            // 等待切分结束
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
            Assert.assertEquals( crudThread.isSuccess(), true,
                    crudThread.getErrorMsg() );
            // 构造源组期望数据
            List< BSONObject > srcExpect = new ArrayList< BSONObject >();
            for ( int i = 0; i < 40000; i++ ) {
                srcExpect.add( ( BSONObject ) JSON
                        .parse( "{sk:" + i + ",alpha:" + i + "}" ) );
            }
            for ( int i = 40000; i < 50000; i++ ) {
                srcExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",beta:2}" ) );
            }
            // 检验源组数据
            checkGroupData( db, srcGroupName, srcExpect );

            // 构造目标组期望数据
            List< BSONObject > destExpect = new ArrayList< BSONObject >();
            for ( int i = 50000; i < 60000; i++ ) {
                destExpect.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",beta:2}" ) );
            }
            for ( int i = 60000; i < 100000; i++ ) {
                destExpect.add( ( BSONObject ) JSON
                        .parse( "{sk:" + i + ",alpha:" + i + "}" ) );
            }
            // 检验目标组数据
            checkGroupData( db, destGroupName, destExpect );

            // 检验主表，查询边界
            List< BSONObject > allInsertedData = new ArrayList< BSONObject >(
                    srcExpect );
            allInsertedData.addAll( destExpect );
            checkMainCL( allInsertedData );

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
            if ( crudThread != null ) {
                crudThread.join();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = commSdb.getCollectionSpace( csName );
            cs.dropCollection( subCLName );
            cs.dropCollection( mainCLName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    private void checkMainCL( List< BSONObject > allInsertedData ) {
        DBCursor cursor1 = null;
        DBCursor cursor2 = null;
        DBCursor cursor3 = null;
        DBCursor cursor4 = null;
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            // 比对所有数据
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( mainCLName );
            cursor1 = cl.query( null, null, "", null );
            while ( cursor1.hasNext() ) {
                BSONObject actual = cursor1.getNext();
                actual.removeField( "_id" );
                Assert.assertEquals( allInsertedData.contains( actual ), true,
                        "insertedData can not find this record:" + actual );
                allInsertedData.remove( actual );
            }
            Assert.assertEquals( allInsertedData.size(), 0,
                    "miss some records:" + allInsertedData );

            // 查询主子表边界
            cursor2 = cl.query( "{sk:0}", null, null, null );
            if ( cursor2.hasNext() ) {
                BSONObject actual = cursor2.getNext();
                actual.removeField( "_id" );
                BSONObject expect = ( BSONObject ) JSON
                        .parse( "{sk:0,alpha:0}" );
                Assert.assertEquals( expect.equals( actual ), true,
                        "expect:" + expect + " actual:" + actual );
                if ( cursor2.hasNext() ) {
                    Assert.fail( "mainCL should not have this record:"
                            + cursor2.getNext() );
                }
            }
            cursor3 = cl.query( "{sk:999}", null, null, null );
            if ( cursor3.hasNext() ) {
                BSONObject actual = cursor3.getNext();
                actual.removeField( "_id" );
                BSONObject expect = ( BSONObject ) JSON
                        .parse( "{sk:999,alpha:999}" );
                Assert.assertEquals( expect.equals( actual ), true,
                        "expect:" + expect + " actual:" + actual );
                if ( cursor3.hasNext() ) {
                    Assert.fail( "mainCL should not have this record:"
                            + cursor3.getNext() );
                }
            }

            // 查询切分边界
            cursor4 = cl.query( "{sk:50000}", null, null, null );
            if ( cursor4.hasNext() ) {
                BSONObject actual = cursor4.getNext();
                actual.removeField( "_id" );
                BSONObject expect = ( BSONObject ) JSON
                        .parse( "{sk:50000,beta:2}" );
                Assert.assertEquals( expect.equals( actual ), true,
                        "expect:" + expect + " actual:" + actual );
                if ( cursor4.hasNext() ) {
                    Assert.fail( "mainCL should not have this record:"
                            + cursor3.getNext() );
                }
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.closeAllCursors();
                db.disconnect();
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
                    .getCollection( subCLName );
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

    class Crud extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection mainCL = sdb.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                mainCL.delete( "{sk:{$gte:40000,$lt:60000}}" );// 删除数据
                Thread.sleep( 500 );
                for ( int i = 40000; i < 60000; i++ ) { // 增加数据
                    mainCL.insert( "{sk:" + i + ",beta:1}" );
                }
                Thread.sleep( 500 );
                mainCL.update( "{sk:{$gte:40000,$lt:60000}}", "{$inc:{beta:1}}",
                        null );// 更新数据
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
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
                DBCollection subCL = cs.getCollection( subCLName );
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
