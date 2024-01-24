package com.sequoiadb.fulltext.killnode;

import java.util.Random;

import org.bson.BSONObject;
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
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-12095:数据操作时主节点异常重启切主
 * @Author zhaoyu
 * @Date 2019-08-05
 */

public class Fulltext12095 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String clName = "cl12095";
    private String indexName = "index12095";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int insertNum = 5000;

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
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        FullTextDBUtils.insertData( cl, insertNum );
        cl.createIndex( indexName, "{a:'text',b:'text',c:'text',d:'text'}",
                false, false );
    }

    @Test
    public void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();

        FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask( master,
                new Random().nextInt( 10 ) );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        mgr.addTask( new InsertThread() );
        mgr.addTask( new UpdateThread() );
        mgr.addTask( new DeleteThread() );
        mgr.addTask( new QueryThread() );
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( FullTextUtils.checkAdapter(), true );

        cl.insert( "{a:'text12095'}" );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
        int expCount = ( int ) cl.getCount();
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, expCount ) );

    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class InsertThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < insertNum; i++ ) {
                    cl.insert( "{a:'fulltext12095_" + i + "'}" );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

    private class UpdateThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.update( null, "{$set:{a:'update'}}", null );
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

    private class DeleteThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.delete( "" );
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

    private class QueryThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor cursor = cl.query(
                        "{\"\":{$Text:{query:{match_all:{}}}}}", null, null,
                        null );
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }
}
