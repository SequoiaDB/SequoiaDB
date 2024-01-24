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
 * @Description seqDB-12098: 数据操作时备节点异常重启
 * @author xiaoni Zhao
 * @date 2019/8/10
 */
public class Fulltext12098 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private DBCollection cl = null;
    private List< String > groupNames = null;
    private String groupName = "";
    private String clName = "cl_12098";
    private String indexName = "fullTextIndex_12098";
    private CollectionSpace cs = null;

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
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group: '" + groupName + "'}" ) );
        cl.createIndex( indexName, "{a:'text'}", false, false );
        FullTextDBUtils.insertData( cl, 10000 );
    }

    @Test
    public void Test() throws Exception {
        NodeWrapper node = groupMgr.getGroupByName( groupName ).getSlave();
        FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask( node, 1 );
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
        FullTextDBUtils.dropCollection( cs, clName );
        sdb.close();
    }

    private class OperationTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = cs.getCollection( clName );
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
