package com.sequoiadb.cappedCL.killnode;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.cappedCL.CappedCLUtils;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-15793:pop操作同时主节点异常重启
 * @Author zhaoyu
 * @Date 2019-7-24
 */
public class CappedCLKillNode15793 extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String cappedCLName = "cappedCL_killNode_15793";
    private String dataGroupName;
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 2000 );
    private int threadNum = 10;

    @BeforeClass
    public void setUp() {
        try {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( cappedCSName );
            cl = cs.createCollection( cappedCLName,
                    ( BSONObject ) JSON.parse( "{Capped:true,Size:1024}" ) );

            // 构造插入的字符串
            strBuffer = new StringBuffer();
            for ( int len = 0; len < stringLength; len++ ) {
                strBuffer.append( "a" );
            }
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( this.getClass().getName()
                    + "setUp error, error description:" + e.getMessage() );
        }

    }

    @Test
    public void test() {
        try {
            dataGroupName = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper dataGroup = groupMgr.getGroupByName( dataGroupName );
            NodeWrapper primaryNode = dataGroup.getMaster();

            FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask(
                    primaryNode.hostName(), primaryNode.svcName(),
                    1 + ( int ) Math.random() * 10 );
            TaskMgr mgr = new TaskMgr( faultMakeTask );
            for ( int i = 0; i < threadNum; i++ ) {
                mgr.addTask( new PopTask() );
                mgr.addTask( new InsertTask() );
            }
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "checkBusinessWithLSN() occurs timeout" );
            Assert.assertEquals( dataGroup.checkInspect( 60 ), true,
                    "data is different on " + dataGroup.getGroupName() );

            BasicBSONObject insertObj = new BasicBSONObject();
            insertObj.put( "a", strBuffer.toString() );
            cl.insert( insertObj );
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "checkBusinessWithLSN() occurs timeout" );
            Assert.assertEquals( dataGroup.checkInspect( 60 ), true,
                    "data is different on " + dataGroup.getGroupName() );
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( "test reliabilityException: " + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( cappedCLName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class PopTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                ArrayList< Long > lids = new ArrayList<>();

                for ( int i = 0; i < 1000; i++ ) {
                    // 获取_id值
                    DBCursor cursor = cl.query( null, null, "{_id:1}", null );
                    while ( cursor.hasNext() ) {
                        long _id = ( long ) cursor.getNext().get( "_id" );
                        lids.add( _id );
                    }
                    cursor.close();

                    // 获取pop的logicalID
                    int pos = 0;
                    long logicalID = 0;
                    if ( lids.size() != 0 ) {
                        pos = new Random().nextInt( lids.size() );
                        logicalID = lids.get( pos );
                    }
                    // pop记录
                    BSONObject popObj = new BasicBSONObject();
                    popObj.put( "LogicalID", logicalID );
                    popObj.put( "Direction", ( pos % 2 == 0 ) ? -1 : 1 );

                    try {
                        // logicalID不存在时，pop失败
                        cl.pop( popObj );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -6 ) {
                            throw e;
                        }
                    }
                }

            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

    private class InsertTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                for ( int i = 0; i < 10000; i++ ) {
                    BasicBSONObject insertObj = new BasicBSONObject();
                    insertObj.put( "a", strBuffer.toString() );
                    cl.insert( insertObj );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

}
