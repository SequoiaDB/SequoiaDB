package com.sequoiadb.metaopr.networkfail.cs;

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
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-4-28
 * @Version 1.00
 */
public class CreateCsSlaver2160 extends SdbTestBase
        implements StandTestInterface {
    List< String > csNames = new ArrayList<>();

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime( this );
        checkBusiness();
        csNames = createNames( "cs2160", 1000 );

    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS( csNames );
        printEndTime( this );
    }

    /**
     * seqDB-2160 创建CS时catalog备节点断网_rlb.netSplit.metaOpr.CS.002
     *
     * 1、创建CS，构造脚本循环执行创建CS操作db.createCS（）
     * 2、删除CS，删除过程中catalog备节点所在主机网络中断（构造网络中断故障，如ifdown网卡） 3、查看CS信息和catalog组内节点状态
     * 4、恢复网络故障（如ifup启动网卡） 5、再次删除相同CS，并在该CS下创建CL，向CL中插入数据
     * 6、查看CS信息（执行db.listCollections（）命令查看CS/CL信息是否和实际一致
     * 7、查看catalog主备节点是否存在该CS相关信息
     */
    @Test
    public void test() throws ReliabilityException {
        DBoperateTask task = DBoperateTask.getTaskCreateCs( csNames );
        String hostName = getSlaveNodeOfCatalog().hostName();
        task.setHostname( CommLib.getSafeCoordUrl( hostName ) );
        FaultMakeTask faultMakeTask = BrokenNetwork.getFaultMakeTask( hostName,
                0, 5 );
        TaskMgr mgr = new TaskMgr( faultMakeTask, task );
        mgr.execute();

        checkBusiness();
        assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        assertTrue( isCsAllCreated( csNames ) );
        // 再次创建，期望成功数量为0
        assertEquals( createCS( csNames ), 0 );
        assertTrue( isCatalogGroupSync() );
    }
}
