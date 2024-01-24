package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ClientOptions;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10529 切分过程中修改CL :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行修改CL操作，分别在如下阶段修改CL:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中修改cl）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加，修改cl中副本数）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组，
 *                       修改cl） 4、查看切分和修改cl操作结果
 * 
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10529C extends SdbTestBase {
    private String csName = "split_10529C";
    private String clName = "cl";
    private CollectionSpace cs;
    private DBCollection cl;
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private List< BSONObject > insertedData = new ArrayList< BSONObject >();
    private AtomicBoolean flag = new AtomicBoolean( false );

    @BeforeClass()
    public void setUp() {
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
        srcGroupName = groupsName.get( 0 );
        destGroupName = groupsName.get( 1 );

        try {
            commSdb.dropCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -34 );
        }
        cs = commSdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                + srcGroupName + "'}" ) );
        // 写入待切分的记录（20000）
        insertData( cl );
    }

    @Test(timeOut = 30 * 60 * 1000)
    public void alterCL() {
        Sequoiadb dataNode = null;
        Split splitThread = null;
        try {
            ClientOptions op = new ClientOptions();
            op.setEnableCache( false );
            Sequoiadb.initClient( op );

            // 启动切分线程
            splitThread = new Split();
            splitThread.start();

            // 等待目标组数据迁移完成
            dataNode = commSdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();//
            // 获得目标组主节点链接
            while ( dataNode.isCollectionSpaceExist( csName ) != true
                    && flag.get() == false ) {
            }
            CollectionSpace dbcs = dataNode.getCollectionSpace( csName );
            while ( dbcs.isCollectionExist( clName ) != true
                    && flag.get() == false ) {
            }
            DBCollection destCL = dbcs.getCollection( clName );
            while ( destCL.getCount() < 12000 && flag.get() == false ) {
            }

            // 修改集合,随机覆盖：1、数据迁移完成，编目未更新；2、数据迁移完成，编目已更新
            Random random = new Random();
            int sleeptime = random.nextInt( 1000 );
            try {
                Thread.sleep( sleeptime );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

            cl.alterCollection( ( BSONObject ) JSON.parse( "{ReplSize:3}" ) );
            // 检查修改结果，replsize 修改为3
            System.out.println( new Date() + " " + this.getClass().getName()
                    + " begin check repl size " );
            CheckReplSize( commSdb, 3 );
            System.out.println( new Date() + " " + this.getClass().getName()
                    + " end check repl size " );
            // check split task
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
        } finally {
            if ( splitThread != null ) {
                splitThread.join();
            }
            if ( dataNode != null ) {
                dataNode.close();
            }
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            commSdb.dropCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }
        }
    }

    private void checkGroupData( Sequoiadb sdb, int expectedCount,
            String macher, int expectTotalCount, String groupName )
            throws Exception {
        Sequoiadb dataNode = null;
        DBCursor cusor = null;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            cusor = cl.query();
            while ( cusor.hasNext() ) {
                BSONObject obj = cusor.getNext();
                Assert.assertEquals( insertedData.contains( obj ), true,
                        "inserted data can not find this record:" + obj );
                insertedData.remove( obj );
            }
            long count = cl.getCount( macher );
            if ( count != expectedCount ) {
                throw new Exception(
                        groupName + " getCount(" + macher + "):expected "
                                + expectedCount + " but found " + count );
            }

            if ( cl.getCount() != expectTotalCount ) {
                throw new Exception( groupName + " getCount:expected "
                        + expectTotalCount + " but found " + cl.getCount() );
            }
        } catch ( Exception e ) {
            throw e;
        } finally {
            if ( dataNode != null ) {
                dataNode.close();
            }
        }
    }

    class Split extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:5000}" ),
                        ( BSONObject ) JSON.parse( "{sk:20000}" ) );
                System.out.println( new Date() + " " + this.getClass().getName()
                        + " begin check group data :" + destGroupName );
                checkGroupData( sdb, 15000, "{sk:{$gte:5000,$lt:20000}}", 15000,
                        destGroupName );
                System.out.println( new Date() + " " + this.getClass().getName()
                        + " begin check group data :" + srcGroupName );
                checkGroupData( sdb, 5000, "{sk:{$gte:0,$lt:5000}}", 5000,
                        srcGroupName );
                System.out.println( new Date() + " " + this.getClass().getName()
                        + " end check group data " );
            } finally {
                flag.set( true );
            }
        }
    }

    // insert 2W records
    private void insertData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 2; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + value
                        + ", test:" + "'testasetatatatatat'" + "}" );
                list.add( obj );
                insertedData.add( obj );
            }
            cl.insert( list );
        }
    }

    private void CheckReplSize( Sequoiadb db, int size ) {
        DBCursor cursor = null;
        try {
            cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            List< BSONObject > tmp = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                tmp.add( cursor.getNext() );
            }
            Assert.assertEquals( tmp.size(), 1, tmp.toString() );
            Assert.assertEquals( ( int ) ( tmp.get( 0 ).get( "ReplSize" ) ),
                    size, tmp.get( 0 ).toString() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }

    }

}
