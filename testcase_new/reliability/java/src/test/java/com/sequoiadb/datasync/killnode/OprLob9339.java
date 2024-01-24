package com.sequoiadb.datasync.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.OprLobTask;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
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
import java.util.Random;

/**
 * @FileName seqDB-9339: lob写入过程中主节点异常重启，选出新主节点，该故障节点为同步的目的节点
 * @Author linsuqiang
 * @Date 2017-03-20
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 2.循环增删LOB 3.过程中构造节点异常重启(kill -9) 4.选主成功后，继续写入 5.过程中故障恢复 6.验证结果
 */

public class OprLob9339 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clName = "cl_9339";
    private String clGroupName = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            db = new Sequoiadb( coordUrl, "", "" );
            groupMgr = GroupMgr.getInstance();

            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            DBCollection cl = createCL( db );
            putLobs( cl ); // prepare data for sync
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
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper priNode = dataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 1 );
            TaskMgr mgr = new TaskMgr( faultTask );
            OprLobTask oTask = new OprLobTask(clName);
            mgr.addTask( oTask );
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
                .parse( "{ ReplSize: 2, Group: '" + clGroupName + "' }" );
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
