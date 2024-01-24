package com.sequoiadb.fulltext.memoryfull;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.faultmodule.fault.FaultName;
import com.sequoiadb.faultmodule.task.FaultTask;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-18963:创建全文索引时主节点适配器内存不足
 * @Author zhaoyu
 * @Date 2019-09-05
 */

public class Fulltext18963 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private String clName = "cl18963";
    private String indexName = "index18963";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int insertNum = 100000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }
        groupMgr = GroupMgr.getInstance();

        groupName = groupMgr.getAllDataGroupName().get( 0 );
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        if ( !FullTextUtils.checkAdapter() ) {
            throw new SkipException( "Check adapter failed" );
        }
        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        FullTextDBUtils.insertData( cl, insertNum );

    }

    @Test
    public void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();
        FaultTask task = FaultTask.getFault( FaultName.MEMORYLIMIT );
        int adaptSvcName = Integer.parseInt( master.svcName() ) + 7;
        try {
            task.make( master.hostName(), String.valueOf( adaptSvcName ),
                    "root", SdbTestBase.rootPwd );
            TaskMgr mgr = new TaskMgr();
            mgr.addTask( new CreateIndexThread() );
            mgr.execute();
            Thread.sleep( 100000 );
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        } finally {
            task.restore();
        }

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
        Assert.assertEquals( FullTextUtils.checkAdapter(), true );

        cl.insert( "{a:'text18963'}" );
        int expCount = ( int ) cl.getCount();
        Assert.assertEquals( expCount, insertNum + 1 );
        if ( cl.isIndexExist( indexName ) ) {
            Assert.assertTrue(
                    FullTextUtils.isIndexCreated( cl, indexName, expCount ) );
        }
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            sdb.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateIndexThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( indexName,
                        "{a:'text',b:'text',c:'text',d:'text'}", false, false );
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }
}
