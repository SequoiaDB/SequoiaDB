package com.sequoiadb.transaction.jdbc2mysql.fault;

import java.sql.SQLException;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;
import com.sequoiadb.transaction.jdbc2mysql.common.TransJDBCBase;
import com.sequoiadb.transaction.jdbc2mysql.common.TransferJDBCTh;
import com.sequoiadb.transaction.jdbc2mysql.common.TransferLargeJDBCTh;

/**
 * @Description seqDB-18522: hash分区表/主子表，转账的过程中异常重启所有数据节点主节点及转账程序执行的coord节点
 * @author yinzhen
 * @date 2019-10-29
 *
 */
public class TransactionLargeJDBC18522 extends TransJDBCBase {
    private String clName = "cl18522";
    private GroupMgr groupMgr;
    private List< String > groupNames;

    @Override
    protected void beforeSetUp() throws ReliabilityException {
        initCL( clName, 10000 );
        groupMgr = GroupMgr.getInstance();
        groupNames = CommLib.getDataGroupNames( sdb );
    }

    @Test
    public void test()
            throws ReliabilityException, InterruptedException, SQLException {
        // 异常重启所有数据节点的主节点及转账程序连接的coord节点
        TaskMgr taskMgr = new TaskMgr();
        for ( String groupName : groupNames ) {
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask task = KillNode.getFaultMakeTask( node, 60 );
            taskMgr.addTask( task );
        }
        NodeWrapper coordNode = TransUtil.getCoordNode(
                new Sequoiadb( TransUtil.getCoordUrl( sdb ), "", "" ) );
        FaultMakeTask task = KillNode.getFaultMakeTask( coordNode, 60 );
        taskMgr.addTask( task );
        TransUtil.setTimeTask( taskMgr, task );

        for ( int i = 0; i < 200; i++ ) {
            taskMgr.addTask( new TransferLargeJDBCTh( clName ) );
        }
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 300 ),
                "GROUP ERROR" );

        // 待集群正常后，查询所有账户的金额总和
        TransferJDBCTh.checkTransResult( clName, getInsertNum() * 10000 );
    }
}
