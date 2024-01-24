package com.sequoiadb.transaction.restartnode;

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
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-20123:更新记录为原值，重启节点
 * @author yinzhen
 * @date 2019-10-30
 *
 */
@Test(groups = { "rc", "rcauto" })
public class Transaction20123 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl20123";
    private GroupMgr groupMgr;
    private String groupName;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( !groupMgr.checkBusiness( TransUtil.ClusterRestoreTimeOut ) ) {
            throw new SkipException( "GROUP ERROR" );
        }
        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        DBCollection cl = sdb.getCollectionSpace( csName ).createCollection(
                clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cl.insert( "{a:100, b:1}" );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
            sdb.close();
        }
    }

    @Test
    public void test() throws ReliabilityException {

        // 开启事务更新记录R1，使用{$inc:0}更新记录
        sdb.beginTransaction();
        DBCollection cl1 = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        cl1.update( "{b:1}", "{$inc:{a:0}}", "" );

        // 回滚事务，重启集合所在组的主节点
        sdb.rollback();
        NodeWrapper node = groupMgr.getGroupByName( groupName ).getMaster();
        FaultMakeTask task = NodeRestart.getFaultMakeTask( node, 1, 3 );
        TaskMgr taskMgr = new TaskMgr( task );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN(
                TransUtil.ClusterRestoreTimeOut ), "GROUP ERROR" );
    }
}
