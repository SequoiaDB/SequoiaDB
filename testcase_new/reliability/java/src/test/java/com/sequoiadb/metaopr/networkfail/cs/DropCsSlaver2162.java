package com.sequoiadb.metaopr.networkfail.cs;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.metaopr.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

import static com.sequoiadb.metaopr.commons.MyUtil.*;
import static org.testng.Assert.assertTrue;

/**
 * @Description seqDB-2162:删除CS时catalog备节点断网_rlb.netSplit.metaOpr.CS.004
 * @Author laojingtang
 * @Date 2017.04.28
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.09.27
 * @version 1.10
 */
public class DropCsSlaver2162 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private int num = 100;
    private List< String > csNames = new ArrayList<>();
    private String csName = "cs_2162";

    @BeforeClass
    public void setup() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }

        for ( int i = 0; i < num; i++ ) {
            String name = csName + "_" + i;
            if ( sdb.isCollectionSpaceExist( name ) )
                sdb.dropCollectionSpace( name );
            sdb.createCollectionSpace( name );
            csNames.add( name );
        }
    }

    /**
     * seqDB-2162 :: 版本: 1 :: 删除CS时catalog备节点断网_rlb.netSplit.metaOpr.CS.004
     * <p>
     * 1、创建CS，构造脚本循环执行创建CS操作db.createCS（） 2、执行删除CS操作（构造脚本循环执行删除CS操作）
     * 3、删除CS时catalog备节点所在主机网络中断（构造网络中断故障，如ifdown网卡） 3、查看CS信息和catalog主节点状态
     * 4、恢复网络故障（如ifup启动网卡） 5、再次执行删除CS操作 6、查看CS信息（执行listCollections（）命令查看CS信息）
     * 8、查看catalog主备节点是否存在该CS相关信息
     */
    @Test
    public void test() throws ReliabilityException {
        // 刪除CS时catalog备节点断网
        String hostname = getSlaveNodeOfCatalog().hostName();
        FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask( hostname, 0,
                10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < csNames.size(); i++ ) {
            mgr.addTask( new DropCS( csNames.get( i ) ) );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );

        // 删除所有CS后进行校验
        for ( int i = 0; i < csNames.size(); i++ ) {
            if ( sdb.isCollectionSpaceExist( csNames.get( i ) ) )
                sdb.dropCollectionSpace( csNames.get( i ) );
        }

        assertTrue( checkCSNotExist( csNames ) );
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class DropCS extends OperateTask {
        private String name;

        private DropCS( String name ) {
            this.name = name;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 刪除CS较快，随机等待10s内时间以验证在不同阶段断网
                int time = new java.util.Random().nextInt( 10000 );
                sleep( time );
                db.dropCollectionSpace( name );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private boolean checkCSNotExist( List< String > csNames ) {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            for ( String name : csNames ) {
                if ( db.isCollectionSpaceExist( name ) ) {
                    System.out.println( "CS " + name + " is exist" );
                    return false;
                }
            }
        }
        return true;
    }

}
