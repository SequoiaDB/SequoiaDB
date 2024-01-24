package com.sequoiadb.split;

import java.util.ArrayList;
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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10528 切分过程中删除CL :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行删除CL操作，分别在如下阶段删除CL:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中删除cl）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加，删除cl）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组，
 *                       删除cl） 4、查看切分和删除cl操作结果 备注此用例验证B
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10528B extends SdbTestBase {
    private String clName = "testcaseCL_10528B";
    private CollectionSpace cs;
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private AtomicBoolean flag = new AtomicBoolean( false );

    @BeforeClass()
    public void setUp() {
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
        cs = commSdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                + srcGroupName + "'}" ) );
        // 写入待切分的记录（30000）
        insertData( cl );
    }

    @Test(timeOut = 30 * 60 * 1000)
    public void test() {
        Sequoiadb dataNode = null;
        Split splitThread = null;
        try {
            // 切分线程启动
            splitThread = new Split();
            splitThread.start();

            // 等待目标组数据上涨
            commSdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            dataNode = commSdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();
            // flag为了防止split线程失败，产生死循环
            while ( dataNode
                    .isCollectionSpaceExist( SdbTestBase.csName ) != true
                    && flag.get() == false ) {
            }

            CollectionSpace datacs = dataNode
                    .getCollectionSpace( SdbTestBase.csName );
            while ( datacs.isCollectionExist( clName ) != true
                    && flag.get() == false ) {
            }

            DBCollection cl = dataNode.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            while ( cl.getCount() == 0 && flag.get() == false ) {
            }

            // 随机在迁移过程中删除CL
            Random random = new Random();
            int sleeptime = random.nextInt( 4000 );
            try {
                Thread.sleep( sleeptime );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            cs.dropCollection( clName );
            Assert.assertEquals( cs.isCollectionExist( clName ), false,
                    " the cl is exist!" );

            // 检测切分线程
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
        } finally {
            if ( dataNode != null ) {
                dataNode.disconnect();
            }
            if ( splitThread != null ) {
                splitThread.join();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
                ;
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    class Split extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName, 90 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -147
                        && e.getErrorCode() != -190
                        && e.getErrorCode() != -243 ) {
                    e.printStackTrace();
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
                flag.set( true );
            }
        }
    }

    // insert 3W records
    private void insertData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 3; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + value
                        + ", test:" + "'testasetatatatatat'" + "}" );
                list.add( obj );
            }
            cl.insert( list );
        }
    }

}
