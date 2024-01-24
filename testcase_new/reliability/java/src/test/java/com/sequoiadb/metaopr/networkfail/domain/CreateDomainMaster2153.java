package com.sequoiadb.metaopr.networkfail.domain;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

import static com.sequoiadb.metaopr.commons.MyUtil.*;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-4-26
 * @Version 1.00
 */
public class CreateDomainMaster2153 extends SdbTestBase
        implements StandTestInterface {
    List< String > domains, csNames;
    final int NUM = 1000;

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime( this );
        checkBusiness();
        domains = createNames( "domain2153", NUM );
        csNames = createNames( "cs2153", NUM );
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS( csNames );
        dropDomains( domains );
        printEndTime( this );
    }

    /**
     * seqDB-2153 :: 版本: 1 ::
     * 创建domain时catalog主节点断网_rlb.netSplit.metaOpr.domain.001
     * <p>
     * 1、创建domian，构造脚本循环执行创建domain操作db.createDomain（）
     * 2、创建domian时catalog主节点所在主机网络中断（构造网络中断故障，如ifdown网卡）
     * 3、查看domain创建结果和catalog主节点状态 4、恢复网络故障（如ifup启动网卡）
     * 5、再次创建domain，并指定该domain创建CS
     * 6、查看domain创建结果（执行db.listDomain命令查看domain/CS信息是否和实际一致
     * 7、查看catalog主备节点是否存在该domain相关信息
     */
    @Test
    public void createDomainMaster()
            throws ReliabilityException, InterruptedException {
        DBoperateTask task = DBoperateTask.getTaskCreateDomains( domains );
        String hostName = getMasterNodeOfCatalog().hostName();
        String safeUrl = CommLib.getSafeCoordUrl( hostName );
        task.setHostname( safeUrl );
        FaultMakeTask faultMakeTask = BrokenNetwork.getFaultMakeTask( hostName,
                1, 5 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask, task );
        taskMgr.execute();

        checkBusiness();
        if ( hostName.equals( getMasterNodeOfCatalog().hostName() ) )
            Thread.sleep( 5 * 60 * 1000 + 10 * 1000 );
        createDomains(
                domains.subList( task.getBreakIndex(), domains.size() ) );
        assertEquals( createDomains( domains ), 0 );
        assertTrue( isDomainAllCreated( domains ) );
        // 指定domain创建cs
        for ( int i = 0; i < NUM; i++ ) {
            createCS( csNames.get( i ), domains.get( i ) );
        }
        assertTrue( isCsAllCreated( csNames ) );
        assertTrue( isCatalogGroupSync() );
    }
}
