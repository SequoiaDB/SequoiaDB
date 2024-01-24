package com.sequoiadb.datasync.killnode;

import java.util.ArrayList;

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
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @testlink seqDB-22006:存在多个唯一索引，插入/更新记录再备节点重放记录与多个桶产生duplicated
 *           key错误，同时强杀数据备节点
 * @author zhaoyu
 * @Date 2020.4.2
 */
public class UniqueIndexReplSyncOptimize22006 extends SdbTestBase {

    private String clName = "cl20249";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private int loopNum = 20000;
    private String groupName;
    private GroupMgr groupMgr;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cs = sdb.getCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        dbcl.createIndex( "a20249", "{a:1}", true, true );
        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:7,a:7,order:1}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:8,a:8,order:2}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:9,a:9,order:3}" ) );
        insertRecords
                .add( ( BSONObject ) JSON.parse( "{_id:10,a:10,order:4}" ) );
        dbcl.insert( insertRecords );
    }

    @Test
    public void test() throws Exception {
        // 异常重启数据备节点
        TaskMgr taskMgr = new TaskMgr();
        GroupWrapper group = groupMgr.getGroupByName( groupName );
        NodeWrapper node = group.getSlave();
        taskMgr.addTask( KillNode.getFaultMakeTask( node, 60 ) );
        taskMgr.addTask( new InsertThread() );
        taskMgr.addTask( new UpdateThread() );
        taskMgr.execute();
        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( group.checkInspect( 60 ), true );

    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    private class InsertThread extends OperateTask {

        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.insert( "{_id:1,a:1}" );
                    cl.insert( "{_id:3,a:2}" );
                    cl.insert( "{_id:5,a:3}" );
                    cl.delete( "{_id:1}" );
                    cl.delete( "{_id:3}" );
                    cl.delete( "{_id:5}" );
                    cl.insert( "{_id:2,a:1}" );
                    cl.insert( "{_id:4,a:2}" );
                    cl.insert( "{_id:6,a:3}" );
                    cl.delete( "{_id:2}" );
                    cl.delete( "{_id:4}" );
                    cl.delete( "{_id:6}" );
                    // analyze会写日志，但是这个日志不会并发重放，验证并发重放转成非并发重放的正确性
                    if ( 0 == i % 1000 ) {
                        BSONObject analyzeOtions = ( BSONObject ) JSON
                                .parse( "{Collection: '" + csName + "." + clName
                                        + "' }" );
                        db.analyze( analyzeOtions );
                    }

                }
            }
        }
    }

    private class UpdateThread extends OperateTask {

        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.update( "{_id:7}", "{$set:{a:17}}", null );
                    cl.update( "{_id:7}", "{$set:{a:7,b:" + i + "}}", null );
                    cl.update( "{_id:8}", "{$set:{a:17}}", null );
                    cl.update( "{_id:8}", "{$set:{a:8,b:" + i + "}}", null );
                    cl.update( "{_id:9}", "{$set:{a:19}}", null );
                    cl.update( "{_id:9}", "{$set:{a:9,b:" + i + "}}", null );
                    cl.update( "{_id:10}", "{$set:{a:100}}", null );
                    cl.update( "{_id:10}", "{$set:{a:10,b:" + i + "}}", null );

                    if ( 0 == i % 1000 ) {
                        BSONObject analyzeOtions = ( BSONObject ) JSON
                                .parse( "{Collection: '" + csName + "." + clName
                                        + "' }" );
                        db.analyze( analyzeOtions );
                    }
                }

            }
        }
    }
}
