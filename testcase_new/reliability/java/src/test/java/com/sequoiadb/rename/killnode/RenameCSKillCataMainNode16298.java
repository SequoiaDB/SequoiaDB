package com.sequoiadb.rename.killnode;

import java.util.ArrayList;
import java.util.List;

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
 * @Description RenameKillMainNode16298.java seqDB-16298:执行renameCS过程中，编目主节点故障
 * @author luweikang
 * @date 2018年11月7日
 */
public class RenameCSKillCataMainNode16298 extends SdbTestBase {

    private List< String > oldCSNameList = new ArrayList<>();
    private List< String > newCSNameList = new ArrayList<>();
    private String oldCSName = "oldcs_16298_B";
    private String newCSName = "newcs_16298_B";
    private String clName = "cl_16298_B";
    private GroupMgr groupMgr = null;
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

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        for ( int i = 0; i < csNum; i++ ) {
            CollectionSpace cs = sdb.createCollectionSpace( oldCSName + i );
            cs.createCollection( clName );
            oldCSNameList.add( oldCSName + i );
            newCSNameList.add( newCSName + i );
        }
        sdb.sync();
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper cataMaster = cataGroup.getMaster();

        // 建立并行任务
        TaskMgr task = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                cataMaster.hostName(), cataMaster.svcName(), 0 );

        Rename renameTask = new Rename();
        task.addTask( faultTask );
        task.addTask( renameTask );

        Assert.assertTrue( task.isAllSuccess(), task.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

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
