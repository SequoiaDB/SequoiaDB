package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

import static com.sequoiadb.metaopr.commons.MyUtil.*;
import static org.testng.Assert.assertTrue;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-4-21
 * @Version 1.00
 */
public class CLOperation extends SdbTestBase implements StandTestInterface {
    final String CSNAME = "cs2295";
    final String DOMAINNAME = "domain2295";
    final long DATASIZE = 1000;
    List< String > clNames = new ArrayList<>( 500 );
    String groupName1, groupName2;

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime( this );
        GroupMgr groupMgr;
        List< String > groupNames = null;
        try {
            groupMgr = GroupMgr.getInstance();
            groupNames = groupMgr.getAllDataGroupName();
        } catch ( ReliabilityException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new SkipException( "" );
        }

        if ( groupNames != null && groupNames.size() >= 2 ) {
            groupName1 = groupNames.get( 0 );
            groupName2 = groupNames.get( 1 );
        }

        MyUtil.createDomain( DOMAINNAME, groupName1, groupName2 );
        MyUtil.createCS( CSNAME, DOMAINNAME );
        for ( int i = 0; i < 500; i++ ) {
            String name = "cl" + i;
            clNames.add( name );
        }
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS( CSNAME );
        dropDomain( DOMAINNAME );
        printEndTime( this );
    }

    @Test
    /**
     * seqDB-2299 :: 版本: 1 :: 删除CL时catalog备节点正常重启
     */
    public void dropClSlaverCataNodeRestart() throws ReliabilityException {
        checkBusiness();
        createClInSingleCs( CSNAME, clNames );
        NodeWrapper slaverNode = getSlaveNodeOfCatalog();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( slaverNode,
                1, 5 );
        TaskMgr taskMgr = TaskMgr.getTaskMgr( faultMakeTask );
        OperateTask task = DBoperateTask.getTaskDropCLInOneCs( clNames,
                CSNAME );
        taskMgr.addTask( task );
        taskMgr.execute();
        assertTrue( taskMgr.isAllSuccess() );
        assertTrue( isClAllDeleted( CSNAME, clNames ) );
        assertTrue( isCatalogGroupSync() );
    }

    @Test
    /**
     * seqDB-2296 :: 版本: 1 :: 创建CL时catalog主节点正常重启（指定Domain）
     */
    public void createClWithDomainMasterRestart() throws ReliabilityException {
        checkBusiness();
        dropCls( CSNAME, clNames );

        NodeWrapper master = getMasterNodeOfCatalog();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( master, 1,
                5 );
        TaskMgr taskMgr = TaskMgr.getTaskMgr( faultMakeTask );
        DBoperateTask task = ( DBoperateTask ) DBoperateTask
                .getTaskCreateCLInOneCs( clNames, CSNAME );
        taskMgr.addTask( task );
        taskMgr.execute();

        if ( taskMgr.isAllSuccess() == true ) {
            MyUtil.throwSkipExeWithoutFaultEnv();
        }
        List< String > list = clNames.subList( task.getBreakIndex(),
                clNames.size() );
        createClInSingleCs( CSNAME, list );
        assertTrue( isClAllCreated( CSNAME, clNames ) );
        assertTrue( isCatalogGroupSync() );
        insertSimpleDataIntoCl( CSNAME, clNames.get( 0 ), 1000 );
        long num = getClCountFromGroupMaster( groupName1, CSNAME,
                clNames.get( 0 ) );
        num += getClCountFromGroupMaster( groupName2, CSNAME,
                clNames.get( 0 ) );
        assertTrue( num == 1000 );
    }

    @Test
    /**
     * seqDB-2297 :: 版本: 1 :: 创建CL时catalog备节点正常重启（指定Domain）
     */
    public void createClWithDomainSlaverRestart() throws ReliabilityException {
        checkBusiness();
        dropCls( CSNAME, clNames );

        NodeWrapper slaver = getSlaveNodeOfCatalog();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( slaver, 0,
                5 );
        TaskMgr taskMg = TaskMgr.getTaskMgr( faultMakeTask );
        taskMg.addTask(
                DBoperateTask.getTaskCreateCLInOneCs( clNames, CSNAME ) );
        taskMg.execute();
        assertTrue( taskMg.isAllSuccess() );
        assertTrue( isClAllCreated( CSNAME, clNames ) );
        assertTrue( isCatalogGroupSync() );
        insertSimpleDataIntoCl( CSNAME, clNames.get( 0 ), 1000 );

        long num = getClCountFromGroupMaster( groupName1, CSNAME,
                clNames.get( 0 ) );
        num += getClCountFromGroupMaster( groupName2, CSNAME,
                clNames.get( 0 ) );
        assertTrue( num == 1000 );
    }
}
