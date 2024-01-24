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

import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

/**
 * @FileName:SEQDB-2437 detachCL过程中catalog备节点异常重启
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class KillNodeSubcl2437 extends SdbTestBase {
    private String mainClName = "testcaseCL2437";
    private List< String > subClName = new ArrayList< String >();
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private GroupMgr groupMgr = null;
    private Sequoiadb commSdb;
    private boolean clearFlag = false;

    @BeforeClass()
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();

            // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }

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
            DBCollection cl = commCS
                    .createCollection( mainClName + "_sub_" + i );
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
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper cataSlave = cataGroup.getSlave();

            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cataSlave.hostName(), cataSlave.svcName(), 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Detach() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusiness( 120 ), true );
            Assert.assertEquals( cataGroup.checkInspect( 60 ), true );

            // 向脱离的子表插入数据
            for ( int i = 0; i < subClName.size(); i++ ) {
                DBCollection cl = commSdb.getCollectionSpace( csName )
                        .getCollection(
                                subClName.get( i ).split( "\\." )[ 1 ] );
                cl.insert( "{sk:32}" );
                Assert.assertEquals( cl.getCount( "{sk:32}" ), 1 );
            }
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
