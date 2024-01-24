package com.sequoiadb.subcl.restartnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.split.killnode.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * @FileName:SEQDB-2430 在主表做基本操作时dataRG主节点正常重启
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class NodeRestartSubcl2430 extends SdbTestBase {
    private String mainClName = "testcaseCL2430";
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private GroupMgr groupMgr = null;
    private Sequoiadb commSdb;
    private boolean clearFlag = false;
    private String subClGroupName;

    @BeforeClass()
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();

            // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }
            subClGroupName = groupMgr.getAllDataGroupName().get( 0 );

            commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            commCS = commSdb.getCollectionSpace( csName );
            mainCL = commCS.createCollection( mainClName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{sk1:1,sk2:1},ShardingType:'range',IsMainCL:true}" ) );
            DBCollection cl = commCS.createCollection( mainClName + "_subcl",
                    ( BSONObject ) JSON.parse(
                            "{ShardingType:'hash',Group:'" + subClGroupName
                                    + "',ShardingKey:{sk1:1,sk2:1}}" ) );
            mainCL.attachCollection( cl.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk1:0,sk2:0},UpBound:{sk1:50000,sk2:50000}}" ) );
            insertData();
        } catch ( ReliabilityException e ) {
            if ( commSdb != null ) {
                commSdb.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    private void insertData() {
        // 再次向主表插入数据
        for ( int i = 0; i < 10000; i++ ) {
            mainCL.insert( "{sk1:" + i + ",sk2:" + i + "}" );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper subCLGroup = groupMgr.getGroupByName( subClGroupName );
            NodeWrapper subCLGroupSalve = subCLGroup.getSlave();
            System.out.println( "Restart Node:" + subCLGroupSalve.hostName()
                    + ":" + subCLGroupSalve.svcName() );

            // 建立并行任务
            FaultMakeTask faultTask = NodeRestart
                    .getFaultMakeTask( subCLGroupSalve, 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Remove() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 120 ), true );
            Assert.assertEquals( subCLGroup.checkInspect( 60 ), true );

            // 再次删除
            mainCL.delete( "{sk1:{$gte:9000}}" );

            // 查询
            Assert.assertEquals( mainCL.getCount(), 0 );
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            commSdb.closeAllCursors();
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                CollectionSpace commCS = commSdb.getCollectionSpace( csName );
                commCS.dropCollection( mainClName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }

        }
    }

    class Remove extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            try {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( mainClName );
                cl.delete( "{sk1:{$gte:0,$lt:3000}}" );
                cl.delete( "{sk1:{$gte:3000,$lt:6000}}" );
                cl.delete( "{sk1:{$gte:6000,$lt:9000}}" );
            } catch ( BaseException e ) {
                throw e;
            }

            finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

}
