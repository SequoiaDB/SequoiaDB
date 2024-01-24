package com.sequoiadb.subcl.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
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
 * @FileName:SEQDB-2439 在主表做基本操作时catalog主节点异常重启
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class KillNodeSubcl2439 extends SdbTestBase {
    private String mainClName = "testcaseCL2439";
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
                            "{ShardingType:'range',Group:'" + subClGroupName
                                    + "',ShardingKey:{sk1:1,sk2:1}}" ) );
            mainCL.attachCollection( cl.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk1:0,sk2:0},UpBound:{sk1:5000,sk2:5000}}" ) );
        } catch ( ReliabilityException e ) {
            if ( commSdb != null ) {
                commSdb.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( Utils.CATA_RG_NAME );
            GroupWrapper subCLGroup = groupMgr.getGroupByName( subClGroupName );
            NodeWrapper cataMaster = cataGroup.getMaster();
            System.out.println( "Kill node:" + cataMaster.hostName() + ":"
                    + cataMaster.svcName() );

            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cataMaster.hostName(), cataMaster.svcName(), 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Insert() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 120 ), true );
            Assert.assertEquals( cataGroup.checkInspect( 60 ), true );
            Assert.assertEquals( subCLGroup.checkInspect( 60 ), true );

            // 再次向主表插入数据
            for ( int i = 0; i < 100; i++ ) {
                mainCL.insert( "{sk1:" + i + ",sk2:" + i + ",flag:2}" );
            }

            // 查询
            Assert.assertEquals( mainCL.getCount( "{$or:[{flag:1},{flag:2}]}" ),
                    5100 );

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

    class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( mainClName );
                for ( int i = 0; i < 5000; i++ ) {
                    cl.insert( "{sk1:" + i + ",sk2:" + i + ",flag:1}" );
                }
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
