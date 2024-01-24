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
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

/**
 * @FileName:SEQDB-2438 detachCL过程中dataRG主节点异常重启
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class KillNodeSubcl2438 extends SdbTestBase {
    private String mainClName = "testcaseCL2438";
    private List< String > subClName = new ArrayList< String >();
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
                    .parse( "{ShardingKey:{'sk':1},ShardingType:'range',IsMainCL:true}" ) );
            createSubCLAndAttach( 500 );
        } catch ( ReliabilityException e ) {
            if ( commSdb != null ) {
                commSdb.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    private void createSubCLAndAttach( int subClCount ) {
        int lowBound = 0;
        for ( int i = 0; i < subClCount; i++ ) {
            DBCollection cl = commCS.createCollection( mainClName + "_sub_" + i,
                    ( BSONObject ) JSON
                            .parse( "{Group:'" + subClGroupName + "'}" ) );
            subClName.add( cl.getFullName() );
            mainCL.attachCollection( cl.getFullName(),
                    ( BSONObject ) JSON.parse( "{LowBound:{sk:" + lowBound
                            + "},UpBound:{sk:" + ( lowBound + 100 ) + "}}" ) );
            lowBound += 100;
        }
    }

    @Test
    public void test() {
        try {
            GroupMgr groupMgr = GroupMgr.getInstance();
            GroupWrapper subclGroup = groupMgr.getGroupByName( subClGroupName );
            NodeWrapper subClGroupMaster = subclGroup.getMaster();
            System.out.println( "Kill node:" + subClGroupMaster.hostName() + ":"
                    + subClGroupMaster.svcName() );
            ;

            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    subClGroupMaster.hostName(), subClGroupMaster.svcName(),
                    0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Detach() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusiness( 120 ), true );
            Assert.assertEquals( subclGroup.checkInspect( 60 ), true );

            // 向主表插入数据并查询
            int lowBound = getMainCLLowBound();
            for ( int i = lowBound; i < 50000; i += 100 ) {
                mainCL.insert( "{sk:" + lowBound + "}" );
            }
            Assert.assertEquals(
                    mainCL.getCount( "{sk:{$gte:" + lowBound + ",$lt:50000}}" ),
                    500 - lowBound / 100 );

            // 向detach的表插入数据并查询(所有子表)
            for ( int i = 0; i < subClName.size(); i++ ) {
                DBCollection cl = commSdb.getCollectionSpace( csName )
                        .getCollection(
                                subClName.get( i ).split( "\\." )[ 1 ] );
                cl.insert( "{sk:23}" );
                Assert.assertEquals( cl.getCount( "{sk:23}" ), 1 );
            }
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            commSdb.closeAllCursors();
        }

    }

    private int getMainCLLowBound() {
        DBCursor cursor = null;
        try {
            cursor = commSdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + mainCL.getFullName() + "\"}", null, null );
            BasicBSONList list = null;
            if ( cursor.hasNext() ) {
                list = ( BasicBSONList ) cursor.getNext().get( "CataInfo" );
            } else {
                Assert.fail( mainCL.getFullName()
                        + " collection catalog not found" );
            }
            if ( list.size() == 0 ) {
                return 50000;
            }
            BSONObject obj = ( BSONObject ) list.get( 0 );
            BSONObject lowBound = ( BSONObject ) obj.get( "LowBound" );
            int low = ( int ) lowBound.get( "sk" );
            return low;
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
        return 0;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                CollectionSpace commCS = commSdb.getCollectionSpace( csName );
                for ( int i = 0; i < subClName.size(); i++ ) {
                    commCS.dropCollection(
                            subClName.get( i ).split( "\\." )[ 1 ] );
                }
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

    class Detach extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                Iterator< String > it = subClName.iterator();
                while ( it.hasNext() ) {
                    mainCL.detachCollection( it.next() );
                }

            } catch ( BaseException e ) {
                System.out.println( "Detach Excepetion:" + e.getErrorCode() );
            }

            finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

}
