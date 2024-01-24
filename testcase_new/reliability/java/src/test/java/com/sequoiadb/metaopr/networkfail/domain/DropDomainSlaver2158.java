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
public class DropDomainSlaver2158 extends SdbTestBase
        implements StandTestInterface {
    List< String > domains = new ArrayList<>();

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime( this );
        checkBusiness();
        domains = createNames( "domain2158", 1000 );
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropDomains( domains );
        printEndTime( this );
    }

    /**
     * seqDB-2158 :: 版本: 1 ::
     * 删除domain时catalog备节点断网_rlb.netSplit.metaOpr.domain.006
     * <p>
     * 1、创建domian，构造脚本循环执行创建domain操作db.createDomain（）
     * 2、执行删除domian操作（构造脚本循环执行删除多个domain操作）
     * 3、删除domain时catalog备节点所在主机网络中断（构造网络中断故障，如ifdown网卡）
     * 3、查看domain信息和catalog组内节点状态 4、恢复网络故障（如ifup启动网卡） 5、再次执行删除domain操作
     * 6、查看domain信息（执行db.listDomain命令查看domain/CS信息是否和实际一致
     * 7、查看catalog主备节点是否存在该domain相关信息
     */

    @Test
    public void dropDomainSlaver() throws ReliabilityException {
        createDomains( domains );

        DBoperateTask task = DBoperateTask.getTaskDropDomains( domains );
        String hostName = getSlaveNodeOfCatalog().hostName();
        task.setHostname( CommLib.getSafeCoordUrl( hostName ) );
        FaultMakeTask faultMakeTask = BrokenNetwork.getFaultMakeTask( hostName,
                0, 5 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask, task );
        taskMgr.execute();

        checkBusiness();
        assertTrue( taskMgr.isAllSuccess() );
        assertTrue( isDomainsDeleted( domains ) );
        // 再次删除，期望成功删除数量为0
        assertTrue( dropDomains( domains ) == 0 );
        assertTrue( isCatalogGroupSync() );
    }

}
