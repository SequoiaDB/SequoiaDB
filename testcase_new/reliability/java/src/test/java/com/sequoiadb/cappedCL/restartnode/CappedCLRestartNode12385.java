package com.sequoiadb.cappedCL.restartnode;

import java.util.ArrayList;
import java.util.List;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.cappedCL.CappedCLUtils;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-12385:插入记录 , 正常重启节点后, 再次插入记录
 * @Author zhaoyu
 * @Date 2019-07-24
 */

public class CappedCLRestartNode12385 extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String cappedCLName = "cappedCL_12385";
    private String groupName = null;
    private int threadNum = 10;
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 2000 );

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( cappedCSName );
        cl = cs.createCollection( cappedCLName,
                ( BSONObject ) JSON.parse( "{Capped:true, Size:10240}" ) );

        // 构造插入的字符串
        strBuffer = new StringBuffer();
        for ( int len = 0; len < stringLength; len++ ) {
            strBuffer.append( "a" );
        }
    }

    @Test
    public void test() throws ReliabilityException {
        // 由于框架中，同一个组内获取的node，使用了同一个连接，通过下述方式使用不同连接停节点，避免使用同一个连接做并发停节点的操作，导致异常
        TaskMgr mgr = new TaskMgr();
        groupName = groupMgr.getAllDataGroupName().get( 0 );
        groupMgr.setSdb( new Sequoiadb( SdbTestBase.coordUrl, "", "" ) );
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        int nodeNum = dataGroup.getNodeNum();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask(
                dataGroup.getNodes().get( 0 ), 1 + new Random().nextInt( 10 ),
                10 );
        mgr.addTask( faultMakeTask );
        List< String > nodes = new ArrayList< String >();
        nodes.add( dataGroup.getNodes().get( 0 ).hostName() );
        for ( int i = 1; i < nodeNum; i++ ) {
            groupMgr.setSdb( new Sequoiadb( SdbTestBase.coordUrl, "", "" ) );
            GroupWrapper dataGroupi = groupMgr.getGroupByName( groupName );
            for ( int j = 0; j < nodeNum; j++ ) {
                NodeWrapper nodesi = dataGroupi.getNodes().get( j );
                if ( !nodes.contains( nodesi.hostName() ) ) {
                    faultMakeTask = NodeRestart.getFaultMakeTask( nodesi,
                            new Random().nextInt( 10 ), 10 );
                    mgr.addTask( faultMakeTask );
                    nodes.add( nodesi.hostName() );
                    break;
                }
            }

        }

        for ( int i = 0; i < threadNum; i++ ) {
            mgr.addTask( new InsertTask() );
        }
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 1200 ), true,
                "check LSN consistency fail" );
        Assert.assertEquals( dataGroup.checkInspect( 120 ), true,
                "data is different on " + dataGroup.getGroupName() );

        BasicBSONObject insertObj = new BasicBSONObject();
        insertObj.put( "a", strBuffer.toString() );
        cl.insert( insertObj );

        // 校验主节点id字段
        Assert.assertTrue( CappedCLUtils.checkLogicalID( sdb, cappedCSName,
                cappedCLName, stringLength ) );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 1200 ), true,
                "check LSN consistency fail" );
        Assert.assertEquals( dataGroup.checkInspect( 120 ), true,
                "data is different on " + dataGroup.getGroupName() );

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

    private class InsertTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                for ( int i = 0; i < 100000; i++ ) {
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
