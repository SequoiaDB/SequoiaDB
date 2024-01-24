package com.sequoiadb.transaction.brokennetwork;

import java.util.List;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;
import com.sequoiadb.transaction.common.TransferTh;

/**
 * @Description seqDB-18519: hash分区表/主子表，转账的过程中部分数据节点主节点断网
 * @author yinzhen
 * @date 2019-6-19
 *
 */
@Test(groups = { "rc", "rcauto" })
public class Transaction18519 extends SdbTestBase {
    private Sequoiadb sdb;
    private String hashCLName = "cl18519_hash";
    private String mainCLName = "cl18519_main";
    private String subCLName1 = "subcl18519_1";
    private String subCLName2 = "subcl18519_2";
    private GroupMgr groupMgr;
    private List< String > groupNames;
    private int sum = 100000000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "ONE GROUP MODE" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( TransUtil.ClusterRestoreTimeOut ) ) {
            throw new SkipException( "GROUP ERROR" );
        }

        // 创建hash分区表/主子表(主表下挂载多个子表，子表覆盖分区表)，replSize设置为1，且已切分到所有组上，切分键为账户字段
        // 并插入数据 10000 个账户，每个账户 10000 元
        TransUtil.createCLsAndInsertData( sdb, csName, hashCLName, mainCLName,
                subCLName1, subCLName2 );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        TransUtil.cleanEnv( sdb, csName, hashCLName, mainCLName );
    }

    @DataProvider(name = "getCL")
    private Object[][] getCLName() {
        return new Object[][] { { hashCLName }, { mainCLName } };
    }

    @Test(dataProvider = "getCL")
    public void test( String clName )
            throws ReliabilityException, InterruptedException {
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
                Assert.assertTrue(
                        groupMgr.checkBusinessWithLSN(
                                TransUtil.ClusterRestoreTimeOut ),
                        "GROUP ERROR" );
            }
        }

        // 部分数据节点的主节点断网
        FaultMakeTask task = null;
        TaskMgr taskMgr = new TaskMgr();
        for ( int i = 0; i < groupNames.size() - 1; i++ ) {
            String groupName = groupNames.get( i );
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            task = BrokenNetwork.getFaultMakeTask( node.hostName(), 60, 10 );
            taskMgr.addTask( task );
        }
        TransUtil.setTimeTask( taskMgr, task );

        for ( int i = 0; i < 200; i++ ) {
            taskMgr.addTask( new TransferTh( csName, clName ) );
        }
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN(
                TransUtil.ClusterRestoreTimeOut ), "GROUP ERROR" );

        // 待集群正常后，查询所有账户的金额总和
        TransUtil.checkSum( sdb, csName, clName, sum,
                this.getClass().getName() );
    }
}
