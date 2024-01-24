package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

import static com.sequoiadb.metaopr.commons.MyUtil.*;
import static org.testng.Assert.assertTrue;

/**
 * 1、创建domian，在域中添加两个数据组（如group1和group2），且设置AutoSplit参数为自动切分
 * 2、更新域属性（执行domain.alter（{Groups:[“group1”，“group3”}）），删除其中一个复制组，添加新复制组 （如group3）
 * 3、更新域属性过程中catalog主节点正常重启（如执行kill -15杀掉节点进程，构造节点正常重启）
 * 3、查看domain更新结果和catalog主节点状态 4、节点启动成功后（查看节点进程存在）
 * 5、再次执行更新domain操作，并指定该domain创建CS/CL
 * 6、向CL插入数据，查看数据是否正确落到该域对应的组内
 * 7、查看catalog主备节点上该domain信息是否正确
 */

/**
 * @FileName seqDB-2286 :: 版本: 1 ::
 *           更新domain时catalog主节点正常重启_rlb.nodeRestart.metaOpr.domain.003
 *           seqDB-2287 :: 版本: 1 ::
 *           更新domain时catalog备节点正常重启_rlb.nodeRestart.metaOpr.domain.004
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 */
public class UpdateDomain2286 extends SdbTestBase {
    private final String CSNAME = "cs2286";
    private final String CLNAME = "cl2286";
    private final String DOMAINNAME = "domain2286";
    List< String > groupNames;
    private Sequoiadb db;

    @BeforeClass
    public void setup() throws ReliabilityException {
        MyUtil.printBeginTime( this );
        db = getSdb();
        groupNames = GroupMgr.getInstance().getAllDataGroupName();
    }

    @Test
    // seqDB-2286 :: 版本: 1 ::
    // 更新domain时catalog主节点正常重启_rlb.nodeRestart.metaOpr.domain.003
    void testMaster() throws ReliabilityException {
        clearEnv();

        NodeWrapper node = getMasterNodeOfCatalog();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( node, 0,
                7 );
        DBoperateTask task = DBoperateTask.getTaskAlterDomain( DOMAINNAME, 1000,
                groupNames );
        TaskMgr taskMgr = TaskMgr.getTaskMgr( faultMakeTask, task );
        taskMgr.execute();

        if ( !taskMgr.isAllSuccess() ) {
            MyUtil.throwSkipExeWithoutFaultEnv();
        }

        assertTrue( db.isDomainExist( DOMAINNAME ) );
        assertTrue( isCatalogGroupSync() );

        alterDomain( DOMAINNAME, groupNames.get( 0 ), groupNames.get( 1 ) );
        createCl( CSNAME, CLNAME );
        deleteAllInCl( CSNAME, CLNAME );
        insertSimpleDataIntoCl( CSNAME, CLNAME, 100 );

        long num = getClCountFromNode( groupNames.get( 0 ) );
        num += getClCountFromNode( groupNames.get( 1 ) );
        assertTrue( num == 100 );
        assertTrue( isCatalogGroupSync() );
    }

    @Test
    // seqDB-2287 :: 版本: 1 ::
    // 更新domain时catalog备节点正常重启_rlb.nodeRestart.metaOpr.domain.004
    void testSlaver() throws ReliabilityException {
        clearEnv();

        NodeWrapper node = getSlaveNodeOfCatalog();
        FaultMakeTask fault = NodeRestart.getFaultMakeTask( node, 0, 5 );
        DBoperateTask task = DBoperateTask.getTaskAlterDomain( DOMAINNAME, 1000,
                groupNames );
        TaskMgr taskMgr = TaskMgr.getTaskMgr( fault, task );
        taskMgr.execute();

        alterDomain( DOMAINNAME, groupNames.get( 0 ), groupNames.get( 2 ) );
        assertTrue( taskMgr.isAllSuccess() );
        assertTrue( isCatalogGroupSync() );
        createCl( CSNAME, CLNAME );
        DBCollection cl = db.getCollectionSpace( CSNAME )
                .getCollection( CLNAME );
        deleteAllInCl( CSNAME, CLNAME );
        insertSimpleDataIntoCl( CSNAME, CLNAME, 100 );

        long num = getClCountFromNode( groupNames.get( 0 ) );
        num += getClCountFromNode( groupNames.get( 2 ) );
        assertTrue( num == 100, String.valueOf( num ) );
        assertTrue( isCatalogGroupSync() );
    }

    // 清理环境
    private void clearEnv() {
        dropCS( CSNAME );
        dropDomain( DOMAINNAME );
        createDomain( DOMAINNAME, groupNames.get( 0 ), groupNames.get( 1 ) );
        createCS( CSNAME, DOMAINNAME );
    }

    private long getClCountFromNode( String groupName )
            throws ReliabilityException {
        try {
            return getClCountFromGroupMaster( groupName, CSNAME, CLNAME );
        } catch ( BaseException e ) {
            return 0;
        }

    }

    @AfterClass
    public void tearDown() {
        dropCS( CSNAME );
        dropDomain( DOMAINNAME );
        MyUtil.closeDb( db );
        MyUtil.printEndTime( this );
    }
}
