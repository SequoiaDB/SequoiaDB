package com.sequoiadb.fulltext.killnode;

import java.util.List;
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
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-15709:数据操作时主节点异常，rebuild流程验证
 * @Author zhaoyu
 * @Date 2019-08-05
 */

public class Fulltext15709 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String[] csNames = { "Acs15709", "acs15709" };
    private String[] clNames = { "cl15709_1", "cl15709_2" };
    private String indexName = "index15709";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int insertNum = 20000;

    @BeforeClass
    public void setUp() throws Exception {
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
        for ( int i = 0; i < csNames.length; i++ ) {
            if ( sdb.isCollectionSpaceExist( csNames[ i ] ) ) {
                sdb.dropCollectionSpace( csNames[ i ] );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csNames[ i ] );
            for ( int j = 0; j < clNames.length; j++ ) {
                DBCollection cl = cs.createCollection( clNames[ j ],
                        ( BSONObject ) JSON
                                .parse( "{Group:'" + groupName + "'}" ) );
                FullTextDBUtils.insertData( cl, insertNum );
                cl.createIndex( indexName,
                        "{a:'text',b:'text',c:'text',d:'text'}", false, false );
                Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName,
                        insertNum ) );
            }
        }
    }

    // SEQUOIADBMAINSTREAM-4798
    // SEQUOIADBMAINSTREAM-4325
    @Test(enabled = false)
    public void test() throws Exception {
        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < csNames.length; i++ ) {
            mgr.addTask( new InsertThread( csNames[ i ], clNames[ 0 ] ) );
            mgr.addTask( new UpdateThread( csNames[ i ], clNames[ 0 ] ) );
            mgr.addTask( new DeleteThread( csNames[ i ], clNames[ 0 ] ) );
            mgr.addTask( new QueryThread( csNames[ i ], clNames[ 0 ] ) );
        }

        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        List< NodeWrapper > nodes = dataGroup.getNodes();
        for ( int i = 0; i < nodes.size(); i++ ) {
            FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask(
                    nodes.get( i ), new Random().nextInt( 10 ) );
            mgr.addTask( faultMakeTask );
        }
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( FullTextUtils.checkAdapter(), true );

        for ( int i = 0; i < csNames.length; i++ ) {
            CollectionSpace cs = sdb.getCollectionSpace( csNames[ i ] );
            for ( int j = 0; j < clNames.length; j++ ) {
                DBCollection cl = cs.getCollection( clNames[ j ] );
                cl.insert( "{a:'text15709'}" );
                int expCount = ( int ) cl.getCount();
                Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName,
                        expCount ) );
            }

        }
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < csNames.length; i++ ) {
                sdb.dropCollectionSpace( csNames[ i ] );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class InsertThread extends OperateTask {
        String csName;
        String clName;

        public InsertThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < insertNum; i++ ) {
                    cl.insert( "{a:'insert15709_" + i + "'}" );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

    private class UpdateThread extends OperateTask {
        String csName;
        String clName;

        public UpdateThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.update( null, "{$set:{a:'update15709'}}", null );
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

    private class DeleteThread extends OperateTask {
        String csName;
        String clName;

        public DeleteThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

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
        String csName;
        String clName;

        public QueryThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

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
