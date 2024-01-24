package com.sequoiadb.metaopr.networkfail.domain;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.task.FaultMakeTask;
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
 * @Date 17-4-28
 * @Version 1.00
 */
public class CreateDomainSlaver2154 extends SdbTestBase
        implements StandTestInterface {
    List< String > domains = new ArrayList<>();
    String groupName1, groupName2;
    List< String > csNames;
    final int NUM = 1000;

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime( this );
        checkBusiness();
        domains = createNames( "domain2154", NUM );
        csNames = createNames( "cs2154", NUM );
        List< String > groupNames;
        try {
            groupNames = GroupMgr.getInstance().getAllDataGroupName();
        } catch ( ReliabilityException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new SkipException( e.getMessage() );
        }
        groupName1 = groupNames.get( 0 );
        groupName2 = groupNames.get( 1 );
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS( csNames );
        dropDomains( domains );
        printEndTime( this );
    }

    /**
     * seqDB-2154 :: 版本: 1 ::
     * 创建domain时catalog备节点断网_rlb.netSplit.metaOpr.domain.002
     * <p>
     * 1、创建domian，构造脚本循环执行创建domain操作db.createDomain（）
     * 2、创建domian时catalog备节点所在主机网络中断（构造网络中断故障，如ifdown网卡）
     * 3、查看domain创建结果和catalog主节点状态 4、恢复网络故障（如ifup启动网卡）
     * 5、再次创建其他domain，并指定该domain创建CS
     * 6、查看domain创建结果（执行db.listDomain命令查看domain/CS信息是否和实际一致
     * 7、查看catalog主备节点是否存在该domain相关信息
     */
    @Test
    public void createDomainSlaver() throws ReliabilityException {
        DBoperateTask task = DBoperateTask.getTaskCreateDomains( domains );
        String brokeNetworkHostname = getSlaveNodeOfCatalog().hostName();
        String safeCoordUrl = CommLib.getSafeCoordUrl( brokeNetworkHostname );
        task.setHostname( safeCoordUrl );
        FaultMakeTask faultMakeTask = BrokenNetwork
                .getFaultMakeTask( brokeNetworkHostname, 0, 5 );
        TaskMgr taskMg = new TaskMgr( faultMakeTask, task );
        taskMg.execute();

        checkBusiness();
        assertTrue( task.isSuccess() );
        assertTrue( isDomainAllCreated( domains ) );
        // 再次创建，期望成功创建数量为0
        assertTrue( createDomains( domains ) == 0 );

        // 指定创建的domain创建cs
        for ( int i = 0; i < NUM; i++ ) {
            createCS( csNames.get( i ), domains.get( i ) );
        }
        assertTrue( isCsAllCreated( csNames ) );
        assertTrue( isCatalogGroupSync() );
    }
}
