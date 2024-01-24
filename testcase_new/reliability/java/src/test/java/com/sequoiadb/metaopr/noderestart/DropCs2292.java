package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.TaskMgr;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

import static org.testng.Assert.assertTrue;

/**
 * 1、创建CS，构造脚本循环执行创建CS操作db.createCS（）
 * 2、执行删除CS操作（构造脚本循环执行删除CS操作）
 * 3、删除CS时catalog主节点正常重启（如执行kill -15杀掉节点进程，构造节点正常重启）
 * 3、查看CS信息和catalog主节点状态
 * 4、节点启动成功后（查看节点进程存在）
 * 5、再次执行删除CS操作
 * 6、查看CS信息（执行listCollections（）命令查看CS信息）
 * 7、查看catalog主备节点是否存在该CS相关信息
 */

/**
 * @FileName seqDB-2293 :: 版本: 1 ::
 *           删除CS时catalog备节点正常重启_rlb.nodeRestart.metaOpr.CS.004 seqDB-2292 ::
 *           版本: 1 :: 删除CS时catalog主节点正常重启_rlb.nodeRestart.metaOpr.CS.003
 *
 * @Author laojingtang
 * @Date 17-4-21
 * @Version 1.00
 */
public class DropCs2292 extends SdbTestBase implements StandTestInterface {
    List< String > csNames = new ArrayList<>( 500 );

    @BeforeClass
    @Override
    public void setup() {
        MyUtil.printBeginTime( this );
        for ( int i = 0; i < 500; i++ ) {
            String name = "cs2292_" + i;
            csNames.add( name );
        }
    }

    @AfterClass
    @Override
    public void tearDown() {

        MyUtil.printEndTime( this );
    }

    @Test
    void testMasterRestart() throws ReliabilityException {
        assertTrue( GroupMgr.getInstance().checkBusiness() );
        MyUtil.createCS( csNames );
        NodeWrapper masterNode = MyUtil.getMasterNodeOfCatalog();
        TaskMgr taskMgr = new TaskMgr(
                NodeRestart.getFaultMakeTask( masterNode, 0, 5 ) );
        DBoperateTask task = DBoperateTask.getTaskDropCs( csNames );
        taskMgr.addTask( task );
        taskMgr.execute();
        if ( taskMgr.isAllSuccess() == true ) {
            MyUtil.throwSkipExeWithoutFaultEnv();
        }
        MyUtil.dropCS(
                csNames.subList( task.getBreakIndex(), csNames.size() ) );
        assertTrue( MyUtil.isCsAllDeleted( csNames ) );
        assertTrue( MyUtil.isCatalogGroupSync() );
    }

    @Test
    void testSlaveRestart() throws ReliabilityException {
        assertTrue( GroupMgr.getInstance().checkBusiness() );
        MyUtil.createCS( csNames );
        NodeWrapper slaveNode = MyUtil.getSlaveNodeOfCatalog();
        TaskMgr taskMgr = new TaskMgr(
                NodeRestart.getFaultMakeTask( slaveNode, 0, 5 ) );
        DBoperateTask task = DBoperateTask.getTaskDropCs( csNames );
        taskMgr.addTask( task );
        taskMgr.execute();
        assertTrue( taskMgr.isAllSuccess() );
        assertTrue( MyUtil.isCsAllDeleted( csNames ) );
        assertTrue( MyUtil.isCatalogGroupSync() );
    }
}
