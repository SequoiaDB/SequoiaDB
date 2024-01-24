package com.sequoiadb.rename.networkfail;

import java.util.ArrayList;
import java.util.List;

import org.bson.BasicBSONObject;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.rename.RenameUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description RenameKillSlaveNode16771.java 执行rename过程中，主节点网络异常
 * @author luweikang
 * @date 2018年11月7日
 */
public class RenameCSNetworkfailNode16771A extends SdbTestBase {

    private List< String > oldCSNameList = new ArrayList<>();
    private List< String > newCSNameList = new ArrayList<>();
    private String oldCSName = "oldcs_16771A";
    private String newCSName = "newcs_16771A";
    private String clName = "cl_16771A";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private int csNum = 20;
    private int completeTimes = 0;
    private String safeCoordUrl;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        for ( int i = 0; i < csNum; i++ ) {
            CollectionSpace cs = sdb.createCollectionSpace( oldCSName + i );
            cs.createCollection( clName,
                    new BasicBSONObject( "Group", groupName ) );
            oldCSNameList.add( oldCSName + i );
            newCSNameList.add( newCSName + i );
        }
        sdb.sync();
        safeCoordUrl = CommLib.getSafeCoordUrl(
                groupMgr.getGroupByName( groupName ).getMaster().hostName() );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper dataMaster = dataGroup.getMaster();
        // 建立并行任务
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( dataMaster.hostName(), 0, 10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        Rename renameTask = new Rename();
        mgr.addTask( renameTask );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        String match = oldCSName;
        CommLib.waitContextClose( sdb, match, 300, false );
        for ( int i = 0; i < oldCSNameList.size(); i++ ) {
            if ( completeTimes < i + 1 ) {
                RenameUtils.retryRenameCS( oldCSNameList.get( i ),
                        newCSNameList.get( i ) );
            }
            RenameUtils.checkRenameCSResult( sdb, oldCSNameList.get( i ),
                    newCSNameList.get( i ), 1 );
        }

        // 插入数据
        for ( int i = 0; i < newCSNameList.size(); i++ ) {
            DBCollection cl = sdb.getCollectionSpace( newCSNameList.get( i ) )
                    .getCollection( clName );
            RenameUtils.insertData( cl, 1000 );
            long actNum = cl.getCount();
            Assert.assertEquals( actNum, 1000, "check record num" );
        }

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < newCSNameList.size(); i++ ) {
                String csName = newCSNameList.get( i );
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
            }
        } finally {
            if ( sdb != null && !sdb.isClosed() ) {
                sdb.close();
            }
        }
    }

    class Rename extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( safeCoordUrl, "", "" )) {
                for ( int i = 0; i < csNum; i++ ) {
                    db.renameCollectionSpace( oldCSNameList.get( i ),
                            newCSNameList.get( i ) );
                    completeTimes++;
                }
            } catch ( BaseException e ) {
                int actErrCode = e.getErrorCode();
                if ( actErrCode != -134 && actErrCode != -15
                        && actErrCode != -116 && actErrCode != -36 ) {
                    throw e;
                }
            }
        }
    }
}
