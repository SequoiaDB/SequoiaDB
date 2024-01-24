package com.sequoiadb.subcl.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
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

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @FileName:SEQDB-2442 在主表做aggregate时dataRG主节点异常重启
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class KillNodeSubcl2442 extends SdbTestBase {
    private String mainClName = "testcaseCL2442";
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
                    .parse( "{ShardingKey:{sk:1},ShardingType:'range',IsMainCL:true}" ) );
            DBCollection cl = commCS.createCollection( mainClName + "_subcl",
                    ( BSONObject ) JSON.parse(
                            "{ShardingType:'hash',ShardingKey:{sk:1},Group:'"
                                    + subClGroupName + "'}" ) );
            mainCL.attachCollection( cl.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk:0},UpBound:{sk:5000}}" ) );
            insertData();
            groupMgr.checkBusinessWithLSN( 120 );
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
        for ( int i = 0; i < 5000; i++ ) {
            mainCL.insert( "{sk:" + i + "}" );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper subCLGroup = groupMgr.getGroupByName( subClGroupName );
            NodeWrapper subCLGroupMaster = subCLGroup.getMaster();
            System.out.println( "Kill node:" + subCLGroupMaster.hostName() + ":"
                    + subCLGroupMaster.svcName() );

            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    subCLGroupMaster.hostName(), subCLGroupMaster.svcName(),
                    0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Aggregate() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusiness( 120 ), true );
            Assert.assertEquals( subCLGroup.checkInspect( 60 ), true );

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

    class Aggregate extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'S'}" ) );
            DBCursor cusor = null;
            try {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( mainClName );
                List< BSONObject > list = new ArrayList< BSONObject >();
                list.add( ( BSONObject ) JSON
                        .parse( "{$match:{sk:{$gte:1000}}}" ) );
                list.add( ( BSONObject ) JSON.parse( "{$project:{sk:1}}" ) );
                list.add( ( BSONObject ) JSON.parse( "{$limit:4000}" ) );
                list.add( ( BSONObject ) JSON.parse( "{$sort:{sk:1}}" ) );
                cusor = cl.aggregate( list );
                int count = 1000;
                while ( cusor.hasNext() ) {
                    BSONObject actual = cusor.getNext();
                    BSONObject expect = ( BSONObject ) JSON
                            .parse( "{sk:" + count + "}" );
                    if ( !actual.equals( expect ) ) {
                        throw new Exception(
                                "actual:" + actual + " expect:" + expect );
                    }
                    count++;
                }
                if ( count != 5000 ) {
                    throw new Exception( "count wrong:" + count );
                }
            } catch ( BaseException e ) {
                // 暂时不抛错，后续考虑把错误码分类，按类型处理
            } finally {
                if ( sdb != null ) {
                    sdb.closeAllCursors();
                    sdb.close();
                }
            }
        }
    }

}
