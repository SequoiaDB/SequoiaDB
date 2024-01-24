package com.sequoiadb.datasync.diskfull;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

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
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.datasync.AddNodeTask;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-3182 : 创建多个唯一索引后，写入文档过程中备节点磁盘满，该备节点为同步的目的节点
 * @Author fanyu
 * @Date 2017-07-31
 * @Version 1.00
 */

/*
 * 1.创建CS，CL，在CL上创建多个唯一索引 2.循环执行增删改操作 3.往副本组中新增节点 4.过程中购造磁盘满(dd购造) 5.继续写入
 * 6.过程中故障恢复
 */
public class CRUDAndAddNodeWithIndex3182 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private final String clName = "cl_3182";
    private String clGroupName = null;
    private TaskMgr mgr = null;
    private GroupWrapper dataGroup = null;
    private String randomHost ;
    private int randomPort ;
    private AddNodeTask aTask = null ;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }
            db = new Sequoiadb( coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );

            List< String > hosts = groupMgr.getAllHosts();
            Random ran = new Random();
            randomHost = hosts.get( ran.nextInt( hosts.size() ) );
            randomPort = ran.nextInt( reservedPortEnd - reservedPortBegin )
                    + reservedPortBegin;
            Utils.makeReplicaLogFull( clGroupName );

            // 设置任务
            dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper slaveNode = dataGroup.getSlave();
            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(
                    slaveNode.hostName(), slaveNode.dbPath(), 0, 10, null, 80 );
            mgr = new TaskMgr( faultTask );
            CRUDTask cTask = new CRUDTask();
            aTask = new AddNodeTask(clGroupName, randomHost, randomPort);
            mgr.addTask( cTask );
            mgr.addTask( aTask );
            // 各个任务各自初始化
            mgr.init();
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        try {
            mgr.start();
            mgr.join();
            mgr.check();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusiness occurs time out" );
            }

            if ( !dataGroup.checkInspect( 1 ) ) {
                Assert.fail(
                        "data is different on " + dataGroup.getGroupName() );
            }
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
            mgr.fini();
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace commCS = db.getCollectionSpace( csName );
            commCS.dropCollection( clName );
            aTask.removeNode();
        } catch ( BaseException | ReliabilityException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class CRUDTask extends OperateTask {
        Sequoiadb db = null;
        DBCollection cl = null;

        @Override
        public void init() {
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                createCL( db );
                db.setSessionAttr( ( BSONObject ) JSON
                        .parse( "{ PreferedInstance: 'M' }" ) );
                cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                createIndexes( cl );
                insertData( cl );
            } catch ( BaseException e ) {
                if ( db != null ) {
                    db.close();
                }
                throw e;
            }
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int repeatTimes = 100000;
                int j = 0;
                int idxNum = 10;
                for ( int i = 50000; i < repeatTimes; i++ ) {
                    BSONObject rec = ( BSONObject ) JSON
                            .parse( "{ a" + j + " : " + i + " }" );
                    cl.insert( rec );

                    BSONObject modifier = ( BSONObject ) JSON.parse( "{ $set:"
                            + "{ a" + j + " : " + repeatTimes + i + " }}" );
                    cl.update( rec, modifier, null );
                    cl.delete( rec );

                    if ( j == idxNum - 1 ) {
                        j = 0;
                    }
                    j++;
                }
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }

        @Override
        public void check() throws ReliabilityException {
            int recNum = 50000;
            int j = 0;
            int idxNum = 10;
            // 随机检查故障前数据
            for ( int i = 0; i < recNum; i++ ) {
                BSONObject randRec = ( BSONObject ) JSON
                        .parse( "{ a" + j + " : " + i + " }" );
                if ( 1 != cl.getCount( randRec ) ) {
                    throw new ReliabilityException(
                            "previous record " + randRec + " not found" );
                }
                if ( j == idxNum - 1 ) {
                    j = 0;
                }
                j++;
            }
            // 恢复后插入数据正常
            BSONObject rec = ( BSONObject ) JSON
                    .parse( "{ c : 'Hello World' }" );
            cl.insert( rec );
            if ( 1 != cl.getCount( rec ) ) {
                throw new ReliabilityException( "fail to insert into cl" );
            }
        }

        @Override
        public void fini() {
            if ( db != null ) {
                db.close();
            }
        }
     
        private DBCollection createCL( Sequoiadb db ) {
            CollectionSpace commCS = db.getCollectionSpace( csName );
            BSONObject option = ( BSONObject ) JSON
                    .parse( "{ Group: '" + clGroupName + "', ReplSize: 1 }" );
            return commCS.createCollection( clName, option );
        }

        private void insertData( DBCollection cl ) {
            List< BSONObject > recs = new ArrayList< BSONObject >();
            int recNum = 50000;
            int j = 0;
            int idxNum = 10;
            for ( int i = 0; i < recNum; i++ ) {
                BSONObject rec = ( BSONObject ) JSON
                        .parse( "{ a" + j + " : " + i + " }" );
                recs.add( rec );
                if ( j == idxNum - 1 ) {
                    j = 0;
                }
                j++;
            }
            cl.insert( recs, DBCollection.FLG_INSERT_CONTONDUP );
        }

        private void createIndexes( DBCollection cl ) {
            int idxNum = 10;
            for ( int i = 0; i < idxNum; i++ ) {
                String idxName = "idx_" + i;
                BSONObject key = ( BSONObject ) JSON
                        .parse( "{ a" + i + " : 1 }" );
                cl.createIndex( idxName, key, true, false, 8 );
            }
        }
        
   } 
}
