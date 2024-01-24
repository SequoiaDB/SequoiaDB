package com.sequoiadb.fulltext.restartnode;

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
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-12099: 数据操作时备节点正常重启
 * @author xiaoni Zhao
 * @date 2019/8/10
 */
public class Fulltext12099 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private DBCollection cl = null;
    private String groupName = "";
    private String clName = "cl_12099";
    private String indexName = "fullTextIndex_12099";

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
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        groupName = groupNames.get( 0 );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cl.createIndex( indexName, "{a:'text'}", false, false );
        FullTextDBUtils.insertData( cl, 10000 );
    }

    @Test
    public void Test() throws Exception {
        NodeWrapper node = groupMgr.getGroupByName( groupName ).getSlave();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( node, 60,
                10 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        taskMgr.addTask( new OperationTask() );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, 10000 ) );
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        FullTextDBUtils.dropCollection( cs, clName );
        sdb.close();
    }

    private class OperationTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            try {
                for ( int i = 0; i < 10; i++ ) {
                    cl.insert( "{a:'fullText_" + i + "'}" );
                    cl.update( "{a:'fullText_" + i + "'}",
                            "{$set: {a:'fullText_" + 10 + i + "'}}", null );
                    cl.delete( "{a:'fullText_" + 10 + i + "'}" );
                    cl.query();
                }
            } finally {
                db.close();
            }
        }
    }
}
