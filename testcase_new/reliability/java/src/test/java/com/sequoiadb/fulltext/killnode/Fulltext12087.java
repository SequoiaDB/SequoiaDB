package com.sequoiadb.fulltext.killnode;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-12087: 创建全文索引时备节点异常重启
 * @author xiaoni Zhao
 * @date 2019/8/9
 */
public class Fulltext12087 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private DBCollection cl = null;
    private List< String > groupNames = null;
    private String groupName = "";
    private String clName = "cl_12087";
    private String indexName = "fullTextIndex_12087";

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "isStandAlone() TRUE, STANDALONE MODE" );
        }
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "checkBusiness() FAIL, GROUP ERROR" );
        }
        if ( !FullTextUtils.checkAdapter() ) {
            throw new SkipException( "Check adapter failed" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        groupName = groupNames.get( 0 );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{'Group':'" + groupName + "'}" ) );
        FullTextDBUtils.insertData( cl, 10000 );
    }

    @Test
    public void Test() throws Exception {
        NodeWrapper node = groupMgr.getGroupByName( groupName ).getSlave();
        FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask( node, 60 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        taskMgr.addTask( new InsertTask() );
        taskMgr.addTask( new CreateIndexTask() );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, 20000 ) );
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        FullTextDBUtils.dropCollection( cs, clName );
        sdb.close();
    }

    private class InsertTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            try {
                FullTextDBUtils.insertData( cl, 10000 );
            } finally {
                db.close();
            }
        }
    }

    private class CreateIndexTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            try {
                cl.createIndex( indexName, "{a:'text'}", false, false );
            } finally {
                db.close();
            }
        }
    }
}
