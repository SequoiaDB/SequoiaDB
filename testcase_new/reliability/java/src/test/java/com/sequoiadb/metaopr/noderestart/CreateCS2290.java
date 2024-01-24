package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.testng.Assert.assertTrue;

/**
 * 1、创建CS，构造脚本循环执行创建domain操作db.createCS（）
 * 2、创建CS时catalog主节点正常重启（如执行kill -15杀掉节点进程，构造节点正常重启）
 * 3、查看CS创建结果和catalog主节点状态
 * 4、节点启动成功后（查看节点进程存在）
 * 5、再次创建同一个CS，并在CS下创建多个CL，向该CS中插入数据
 * 6、查看CS信息（执行db.listCollections（）命令查看domain/CS信息是否和实际一致
 * 7、查看catalog主备节点是否存在该CS相关信息
 *
 * @FileName seqDB-2290 :: 版本: 1 :: 创建CS时catalog主节点正常重启_rlb.nodeRestart.metaOpr.CS.001
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 * @FileName seqDB-2290 :: 版本: 1 :: 创建CS时catalog主节点正常重启_rlb.nodeRestart.metaOpr.CS.001
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 * @FileName seqDB-2290 :: 版本: 1 :: 创建CS时catalog主节点正常重启_rlb.nodeRestart.metaOpr.CS.001
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 * @FileName seqDB-2290 :: 版本: 1 :: 创建CS时catalog主节点正常重启_rlb.nodeRestart.metaOpr.CS.001
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 */

/**
 * @FileName seqDB-2290 :: 版本: 1 :: 创建CS时catalog主节点正常重启_rlb.nodeRestart.metaOpr.CS.001
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 */

/**
 * 1、创建CS，构造脚本循环执行创建domain操作db.createCS（） 2、创建CS时catalog主节点正常重启（如执行kill
 * -15杀掉节点进程，构造节点正常重启） 3、查看CS创建结果和catalog主节点状态 4、节点启动成功后（查看节点进程存在）
 * 5、再次创建同一个CS，并在CS下创建多个CL，向该CS中插入数据
 * 6、查看CS信息（执行db.listCollections（）命令查看domain/CS信息是否和实际一致
 * 7、查看catalog主备节点是否存在该CS相关信息
 */
public class CreateCS2290 extends SdbTestBase implements StandTestInterface {
    private Sequoiadb db;
    List< String > csNames = new ArrayList<>();

    @BeforeClass
    @Override
    public void setup() {
        MyUtil.printBeginTime( this );
        db = MyUtil.getSdb();
        for ( int i = 0; i < 1000; i++ ) {
            String name = "cs2290_" + i;
            csNames.add( name );
        }
    }

    @AfterClass
    @Override
    public void tearDown() {
        for ( String name : csNames ) {
            try {
                db.dropCollectionSpace( name );
            } catch ( BaseException e ) {
            }
        }
        MyUtil.closeDb( db );
        MyUtil.printEndTime( this );
    }

    @Test
    public void testMaster() throws ReliabilityException {
        assertTrue( GroupMgr.getInstance().checkBusiness() );
        FaultMakeTask faultMakeTask = NodeRestart
                .getFaultMakeTask( MyUtil.getMasterNodeOfCatalog(), 0, 5 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        CreateCSTask csTask = new CreateCSTask( 10 );
        taskMgr.addTask( csTask );
        taskMgr.execute();
        if ( taskMgr.isAllSuccess() == true ) {
            MyUtil.throwSkipExeWithoutFaultEnv();
        }

        MyUtil.createCS(
                csNames.subList( csTask.getBrokenIndex(), csNames.size() ) );
        assertTrue( MyUtil.isCsAllCreated( csNames ) );
        assertTrue( MyUtil.isCatalogGroupSync() );
        MyUtil.createClInManyCs( csNames.subList( 0, 10 ), "cl" );
        assertTrue( isClCreated() );
    }

    @Test
    void testSlaver() throws ReliabilityException {
        assertTrue( GroupMgr.getInstance().checkBusiness() );
        FaultMakeTask faultMakeTask = NodeRestart
                .getFaultMakeTask( MyUtil.getSlaveNodeOfCatalog(), 0, 5 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        CreateCSTask csTask = new CreateCSTask( 0 );
        taskMgr.addTask( csTask );
        taskMgr.execute();
        assertTrue( taskMgr.isAllSuccess() );
        MyUtil.createClInManyCs( csNames.subList( 0, 10 ), "cl" );
        assertTrue( isClCreated() );
        assertTrue( MyUtil.isCatalogGroupSync() );
    }

    class CreateCSTask extends OperateTask {
        int brokenIndex = 0;
        int delayMillis = 0;

        public CreateCSTask( int delayMillis ) {
            this.delayMillis = delayMillis;
        }

        public int getBrokenIndex() {
            return brokenIndex;
        }

        @Override
        public void exec() {
            try ( Sequoiadb db = MyUtil.getSdb()) {
                MyUtil.dropCS( csNames );
                for ( int i = 0; i < csNames.size(); i++ ) {
                    Thread.sleep( delayMillis );
                    db.createCollectionSpace( csNames.get( i ) );
                    brokenIndex = i;
                }
            } catch ( InterruptedException e ) {
            }
        }
    }

    @Deprecated
    boolean isClCreated() {
        DBCursor cursor = db.listCollections();
        Map< String, String > map = new HashMap<>();
        while ( cursor.hasNext() ) {
            String name = cursor.getNext().get( "Name" ).toString();
            map.put( name, "" );
        }
        for ( int i = 0; i < 10; i++ ) {
            String name = csNames.get( i ) + ".cl";
            if ( map.containsKey( name ) == false )
                return false;
        }
        return true;
    }
}
