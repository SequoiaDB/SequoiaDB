package com.sequoiadb.transaction.jdbc2mysql.fault;

import java.sql.SQLException;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;
import com.sequoiadb.transaction.jdbc2mysql.common.TransJDBCBase;
import com.sequoiadb.transaction.jdbc2mysql.common.TransferJDBCTh;

/**
 * @Description seqDB-18518: hash分区表/主子表，转账的过程中所有数据节点主节点断网
 * @author yinzhen
 * @date 2019-8-13
 *
 */
public class TransactionJDBC18518 extends TransJDBCBase {
    private String clName = "cl18518";
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
        // 如果构造断网的主机是连接的coord节点所在的主机，就重启该主节点
        for ( int i = 0; i < groupNames.size(); i++ ) {
            GroupWrapper groupWrapper = groupMgr
                    .getGroupByName( groupNames.get( i ) );
            String host = groupWrapper.getMaster().hostName();
            if ( host.equals( sdb.getHost() ) ) {
                NodeWrapper nodeWrapper = groupWrapper.getMaster();
                FaultMakeTask task = NodeRestart.getFaultMakeTask( nodeWrapper,
                        0, 0 );
                TaskMgr taskMgr = new TaskMgr( task );
                taskMgr.execute();

                Assert.assertTrue( taskMgr.isAllSuccess(),
                        taskMgr.getErrorMsg() );
                Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ),
                        "GROUP ERROR" );
            }
        }

        // 所有数据节点的主节点断网
        TaskMgr taskMgr = new TaskMgr();
        FaultMakeTask task = null;
        for ( String groupName : groupNames ) {
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            task = BrokenNetwork.getFaultMakeTask( node.hostName(), 60, 10 );
            taskMgr.addTask( task );
        }
        TransUtil.setTimeTask( taskMgr, task );

        for ( int i = 0; i < 200; i++ ) {
            taskMgr.addTask( new TransferJDBCTh( clName ) );
        }
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 300 ),
                "GROUP ERROR" );

        // 待集群正常后，查询所有账户的金额总和
        TransferJDBCTh.checkTransResult( clName, getInsertNum() * 10000 );
    }
}
