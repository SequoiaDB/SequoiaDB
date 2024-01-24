package com.sequoiadb.transaction.jdbc2mysql.fault;

import java.sql.SQLException;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;
import com.sequoiadb.transaction.jdbc2mysql.common.TransJDBCBase;
import com.sequoiadb.transaction.jdbc2mysql.common.TransferJDBCTh;
import com.sequoiadb.transaction.jdbc2mysql.common.TransferLargeJDBCTh;

/**
 * @Description seqDB-18599:hash分区表/主子表，转账过程中数据节点磁盘满
 * @author yinzhen
 * @date 2019-10-29
 *
 */
public class TransactionLargeJDBC18599 extends TransJDBCBase {
    private String clName = "cl18599";
    private GroupMgr groupMgr;
    private List< String > groupNames;

    @Override
    protected void beforeSetUp() throws ReliabilityException {
        initCL( clName, 10000 );
        groupMgr = GroupMgr.getInstance();
        groupNames = CommLib.getDataGroupNames( sdb );
    }

    @Override
    protected void afterSetUp() throws ReliabilityException {
        // 如果磁盘满的主机不是同时拥有主备节点，就重启其中一个节点
        String hostName = groupMgr.getGroupByName( groupNames.get( 0 ) )
                .getMaster().hostName();
        for ( int i = 1; i < groupNames.size(); i++ ) {
            GroupWrapper groupWrapper = groupMgr
                    .getGroupByName( groupNames.get( i ) );
            String host = groupWrapper.getMaster().hostName();
            if ( !hostName.equals( host ) ) {
                break;
            }
            if ( i == groupNames.size() - 1 ) {
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
    }

    @Test
    public void test()
            throws ReliabilityException, InterruptedException, SQLException {
        // 构造磁盘主节点/备节点磁盘满
        TaskMgr taskMgr = new TaskMgr();
        GroupWrapper group = groupMgr.getGroupByName( groupNames.get( 0 ) );
        NodeWrapper node = group.getMaster();
        FaultMakeTask task = DiskFull.getFaultMakeTask( node.hostName(),
                SdbTestBase.reservedDir, 60, 10 );
        taskMgr.addTask( task );
        TransUtil.setTimeTask( taskMgr, task );

        for ( int i = 0; i < 200; i++ ) {
            taskMgr.addTask( new TransferLargeJDBCTh( clName ) );
        }
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ),
                "GROUP ERROR" );

        // 待磁盘故障恢复正常后，查询所有账户的金额总和
        TransferJDBCTh.checkTransResult( clName, getInsertNum() * 10000 );
    }
}
