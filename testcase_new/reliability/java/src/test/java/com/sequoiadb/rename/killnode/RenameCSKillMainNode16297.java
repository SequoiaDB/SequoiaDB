package com.sequoiadb.rename.killnode;

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
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.rename.RenameUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description RenameKillMainNode16297.java seqDB-16297:执行rename cs过程中，数据主节点故障
 * @author luweikang
 * @date 2018年11月7日
 */
public class RenameCSKillMainNode16297 extends SdbTestBase {

    private List< String > oldCSNameList = new ArrayList<>();
    private List< String > newCSNameList = new ArrayList<>();
    private String oldCSName = "oldcs_16297B";
    private String newCSName = "newcs_16297B";
    private String clName = "cl_16297B";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private int csNum = 20;
    private int completeTimes = 0;

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
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper dataMaster = dataGroup.getMaster();

        // 建立并行任务
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                dataMaster.hostName(), dataMaster.svcName(), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );
        Rename renameTask = new Rename();
        mgr.addTask( renameTask );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        System.out.println( "completeTimes: " + completeTimes );
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
            if ( sdb != null ) {
                sdb.close();
            }

        }
    }

    class Rename extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < oldCSNameList.size(); i++ ) {
                    db.renameCollectionSpace( oldCSNameList.get( i ),
                            newCSNameList.get( i ) );
                    completeTimes++;
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }
}
