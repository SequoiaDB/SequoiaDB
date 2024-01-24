package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.TaskMgr;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.sequoiadb.metaopr.commons.MyUtil.*;
import static org.testng.Assert.assertTrue;

/**
 * 1、创建domian，构造脚本循环执行创建domain操作db.createDomain（）
 * 2、执行删除domian操作（构造脚本循环执行删除多个domain操作）
 * 3、删除domain时catalog主节点正常重启（如执行kill -15杀掉节点进程，构造节点正常重启）
 * 3、查看domain信息和catalog主节点状态 4、节点启动成功后（查看节点进程存在）
 * 5、再次执行删除domain操作
 * 6、查看domain信息（执行db.listDomain命令查看domain/CS信息是否和实际一致
 * 7、查看catalog主备节点是否存在该domain相关信息
 */

/**
 * @FileName seqDB-2288 :: 版本: 1 ::
 *           删除domain时catalog主节点正常重启_rlb.nodeRestart.metaOpr.domain.005
 *           seqDB-2289 :: 版本: 1 ::
 *           删除domain时catalog备节点正常重启_rlb.nodeRestart.metaOpr.domain.006
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 */
public class DropDomain2288 extends SdbTestBase {
    private List< String > domainNames = new ArrayList<>( 200 );
    private Map< String, String > domainNamesMap = new HashMap<>();

    private Sequoiadb db;

    @BeforeClass
    public void setup() {
        MyUtil.printBeginTime( this );
        db = MyUtil.getSdb();
        for ( int i = 0; i < 500; i++ ) {
            String name = "domain" + i;
            domainNames.add( name );
            domainNamesMap.put( name, "" );
        }
    }

    @AfterClass
    public void tearDown() {
        MyUtil.closeDb( db );
        MyUtil.printEndTime( this );
    }

    @Test
    // 测试备节点正常重启
    void testSlave() throws ReliabilityException {
        assertTrue( GroupMgr.getInstance().checkBusiness() );
        createDomains( domainNames );
        TaskMgr taskMgr = new TaskMgr( NodeRestart
                .getFaultMakeTask( MyUtil.getSlaveNodeOfCatalog(), 2, 5 ) );
        taskMgr.addTask( DBoperateTask.getTaskDropDomains( domainNames, 10 ) );
        taskMgr.execute();
        assertTrue( taskMgr.isAllSuccess() );
        assertTrue( isCatalogGroupSync() );
        assertTrue( isDomainsDeleted( domainNames ) );
    }

    @Test
    // 测试主节点正常重启
    void testMaster() throws ReliabilityException {
        assertTrue( GroupMgr.getInstance().checkBusiness() );
        createDomains( domainNames );
        TaskMgr taskMgr = new TaskMgr( NodeRestart
                .getFaultMakeTask( MyUtil.getMasterNodeOfCatalog(), 0, 5 ) );
        DBoperateTask task = DBoperateTask.getTaskDropDomains( domainNames,
                10 );
        taskMgr.addTask( task );
        taskMgr.execute();
        if ( taskMgr.isAllSuccess() == true ) {
            MyUtil.throwSkipExeWithoutFaultEnv();
        }
        for ( int i = task.getBreakIndex(); i < domainNames.size(); i++ ) {
            try {
                db.dropDomain( domainNames.get( i ) );
            } catch ( BaseException e ) {
            }
        }

        assertTrue( isDomainsDeleted( domainNames ) );
        assertTrue( isCatalogGroupSync() );
    }
}
