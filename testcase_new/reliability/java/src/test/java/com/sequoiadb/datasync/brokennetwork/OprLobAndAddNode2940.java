package com.sequoiadb.datasync.brokennetwork;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.OprLobTask;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.datasync.AddNodeTask;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;
import java.util.List;
import java.util.Random;

/**
 * @FileName seqDB-2940: LOB写入加新建节点过程中主节点断网，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-20
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 2.循环增删LOB 3.往副本组中新增节点 3.过程中构造断网故障(例如：ifdown) 4.选主成功后，继续写入部分LOB
 * 5.过程中故障恢复 (例如：ifup) 6.验证结果
 */

public class OprLobAndAddNode2940 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clName = "cl_2940";
    private String clGroupName = null;
    private String randomHost = null;
    private int randomPort;
    private GroupWrapper dataGroup = null;
    private String dataPriHost = null;
    private AddNodeTask aTask = null ;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {
            groupMgr = GroupMgr.getInstance();

            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            String cataPriHost = cataGroup.getMaster().hostName();
            dataGroup = groupMgr.getGroupByName( clGroupName );
            dataPriHost = dataGroup.getMaster().hostName();
            if ( cataPriHost.equals( dataPriHost )
                    && !cataGroup.changePrimary() ) {
                throw new SkipException(
                        cataGroup.getGroupName() + " reelect fail" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            DBCollection cl = createCL( db );
            putLobs( cl ); // prepare data for sync

            // node info, which will be used at AddNodeTask and teardown
            Random ran = new Random();
            List< String > hosts = groupMgr.getAllHosts();
            randomHost = hosts.get( ran.nextInt( hosts.size() ) );
            randomPort = ran.nextInt( reservedPortEnd - reservedPortBegin )
                    + reservedPortBegin;

            Utils.makeReplicaLogFull( clGroupName );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( dataPriHost, 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            String safeUrl = CommLib.getSafeCoordUrl( dataPriHost );
            OprLobTask oTask = new OprLobTask(safeUrl, clName);
            aTask = new AddNodeTask( clGroupName, randomHost,
                    randomPort );
            mgr.addTask( oTask );
            mgr.addTask( aTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusiness occurs timeout" );
            }

            if ( !dataGroup.checkInspect( 1 ) ) {
                Assert.fail(
                        "data is different on " + dataGroup.getGroupName() );
            }
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = db.getCollectionSpace( csName );
            cs.dropCollection( clName );
            aTask.removeNode();
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private DBCollection createCL( Sequoiadb db ) {
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ ReplSize: 1, Group: '" + clGroupName + "' }" );
        CollectionSpace cs = db.getCollectionSpace( csName );
        return cs.createCollection( clName, option );
    }

    private void putLobs( DBCollection cl ) {
        int lobSize = 1 * 1024 * 1024;
        byte[] lobBytes = new byte[ lobSize ];
        new Random().nextBytes( lobBytes );

        int lobNum = 100;
        for ( int i = 0; i < lobNum; i++ ) {
            DBLob lob = cl.createLob();
            lob.write( lobBytes );
            lob.close();
        }
    }

    
}
