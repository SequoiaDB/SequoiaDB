package com.sequoiadb.metaopr.networkfail.cl;

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
public class CreateClMaster2165 extends SdbTestBase
        implements StandTestInterface {
    private final String domain = "domain2165";
    private final String csName = "cs2165";
    private List< String > clNames;

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime( this );
        checkBusiness();
        clNames = createNames( "cl2165", 1000 );
        List< String > groupnames;
        try {
            groupnames = GroupMgr.getInstance().getAllDataGroupName();
        } catch ( ReliabilityException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new SkipException( e.getMessage() );
        }
        createDomain( domain, groupnames.get( 0 ), groupnames.get( 1 ) );
        createCS( csName, domain );
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS( csName );
        dropDomain( domain );
        printEndTime( this );
    }

    /**
     * seqDB-2165 :: 版本: 1 ::
     * 创建CL时catalog主节点断网（指定Domain）_rlb.netSplit.metaOpr.CL.003
     * <p>
     * 1、创建Domain，在域中添加两个及以上的数据组 2、创建CS并指定到这个域，在该CS下创建CL（执行脚本构造循环执行创建多个CL操作）
     * 3、创建CL时catalog主节点所在主机网络中断（构造网络中断故障，如ifdown网卡） 3、查看CL创建结果和catalog主节点状态
     * 4、恢复网络故障（如ifup启动网卡） 5、再次创建相同CL，向该CL中插入数据
     * 6、查看CL信息（执行db.listCollections（）命令查看CS/CL信息是否和实际一致）
     * 7、查看catalog主备节点是否存在该CS相关信息
     */
    @Test
    public void test() throws ReliabilityException, InterruptedException {
        DBoperateTask task = DBoperateTask.getTaskCreateCLInOneCs( clNames,
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
        // 从断点开始再次创建cl
        createClInSingleCs( csName,
                clNames.subList( task.getBreakIndex(), clNames.size() ) );
        assertEquals( createClInSingleCs( csName, clNames ), 0 );
        assertTrue( isClAllCreated( csName, clNames ) );
        assertTrue( isCatalogGroupSync() );
    }
}
