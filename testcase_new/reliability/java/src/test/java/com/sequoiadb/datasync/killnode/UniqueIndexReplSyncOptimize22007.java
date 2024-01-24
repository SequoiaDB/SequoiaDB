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
 * @testlink seqDB-22007:更新重复键及_id字段，同时强杀数据备节点
 * @author zhaoyu
 * @Date 2020.4.2
 */
public class UniqueIndexReplSyncOptimize22007 extends SdbTestBase {

    private String clName = "cl20250";
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
        dbcl.createIndex( "a20250", "{a:1}", true, true );
        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:2,a:1,order:1}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:3,a:2,order:2}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:4,a:4,order:3}" ) );
        dbcl.insert( insertRecords );
    }

    @Test
    public void test() throws Exception {
        // 异常重启数据备节点
        TaskMgr taskMgr = new TaskMgr();
        GroupWrapper group = groupMgr.getGroupByName( groupName );
        NodeWrapper node = group.getSlave();
        taskMgr.addTask( KillNode.getFaultMakeTask( node, 60 ) );
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

    private class UpdateThread extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.update( "{_id:2}", "{$set:{a:15}}", null );
                    cl.update( "{_id:2}", "{$set:{a:1,b:" + i + "}}", null );
                    cl.update( "{_id:4}", "{$set:{_id:5,a:5,b:" + i + "}}",
                            null );
                    cl.update( "{_id:3}", "{$set:{a:15}}", null );
                    cl.update( "{_id:3}", "{$set:{a:2,b:" + i + "}}", null );
                    cl.update( "{_id:5}", "{$set:{_id:4,a:4,b:" + i + "}}",
                            null );
                }
            }
        }
    }
}
