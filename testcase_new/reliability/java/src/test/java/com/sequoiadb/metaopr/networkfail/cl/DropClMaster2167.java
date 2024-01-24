package com.sequoiadb.metaopr.networkfail.cl;

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
import static org.testng.Assert.assertTrue;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-4-28
 * @Version 1.00
 */
public class DropClMaster2167 extends SdbTestBase
        implements StandTestInterface {
    final String csName = "cs2167";
    List< String > clnames;

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime( this );
        checkBusiness();
        createCS( csName );
        clnames = createNames( "cl2167", 1000 );
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS( csName );
        printEndTime( this );
    }

    /**
     * seqDB-2167 :: 版本: 1 :: 删除CL时catalog主节点断网_rlb.netSplit.metaOpr.CL.005
     * <p>
     * 1、创建CS，在该CS下创建多个CL 2、执行删除CL操作（构造脚本循环执行删除CL操作）
     * 3、删除CL时catalog主节点所在主机网络中断（构造网络中断故障，如ifdown网卡） 3、查看CL信息和catalog主节点状态
     * 4、恢复网络故障（如ifup启动网卡） 5、再次执行删除CL操作 6、查看CL信息（执行listCollections（）命令查看CL信息）
     * 9、查看catalog主备节点是否存在该CL相关信息
     */
    @Test
    public void test() throws ReliabilityException, InterruptedException {
        createClInSingleCs( csName, clnames );

        DBoperateTask task = DBoperateTask.getTaskDropCLInOneCs( clnames,
                csName );
        String hostname = getMasterNodeOfCatalog().hostName();
        String safeUrl = CommLib.getSafeCoordUrl( hostname );
        task.setHostname( safeUrl );
        FaultMakeTask faultMakeTask = BrokenNetwork.getFaultMakeTask( hostname,
                0, 5 );
        TaskMgr mgr = new TaskMgr( faultMakeTask, task );
        mgr.execute();

        checkBusiness();
        if ( hostname.equals( getMasterNodeOfCatalog().hostName() ) )
            Thread.sleep( 5 * 60 * 1000 + 10 * 1000 );
        dropCls( csName,
                clnames.subList( task.getBreakIndex(), clnames.size() ) );
        assertTrue( isClAllDeleted( csName, clnames ) );
        assertTrue( isCatalogGroupSync() );
    }
}
