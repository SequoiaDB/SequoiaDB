package com.sequoiadb.index;

import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-8-8
 * @Version 1.00
 */
public class Index4118 extends IndexTestBase {

    @BeforeClass
    @Override
    public void setup() {
        super.setup();
        BSONObject option = ( BSONObject ) JSON.parse(
                "{ ShardingKey: { \"age\": 1 }," + " ShardingType: \"hash\", "
                        + "Partition: 1024, ReplSize: 1,"
                        + " Compressed: true ," + "Group:\"group1\"}" );
        createIndexCl( option );
        insertData();
        split50();
        createIndexes( indexAlreadlyCreated );
    }

    /**
     * [显示测试用例规格] 测试用例 seqDB-4118 :: 版本: 1 ::
     * 在切分表上，创建/删除索引时，group1备节点异常重启_rlb.nodeAbnoRestart.basicOpe.006
     */
    @Test
    public void test() throws ReliabilityException {
        NodeWrapper node = GroupMgr.getInstance().getGroupByName( "group1" )
                .getSlave();
        TaskMgr taskMgr = new TaskMgr( KillNode
                .getFaultMakeTask( node.hostName(), node.svcName(), 0 ) );
        IndexTask createTask = getCreateTask();
        IndexTask deleteTask = getDeleteTask();

        taskMgr.addTask( createTask ).addTask( deleteTask );

        taskMgr.execute();
        MyUtil.checkBusiness();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );

        Assert.assertTrue( isIndexesAllCreatedInNodes( "group1", "group2" ) );
        Assert.assertTrue( isIndexesAllDeletedInNodes( "group1", "group2" ) );

    }
}
