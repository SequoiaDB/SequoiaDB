package com.sequoiadb.fulltext.killnode;

import java.util.ArrayList;
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
 * @Description seqDB-14495:删除集合空间时备节点异常重启
 * @author xiaoni Zhao
 * @date 2019/8/10
 */
public class Fulltext14495 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private List< String > cappedClNames = new ArrayList< String >();
    private List< String > esIndexNames = new ArrayList< String >();

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
        cs = sdb.getCollectionSpace( csName );
        for ( int i = 0; i < 10; i++ ) {
            DBCollection cl = cs.createCollection( "cl_14495_" + i,
                    ( BSONObject ) JSON
                            .parse( "{'Group':'" + groupName + "'}" ) );
            cl.createIndex( "fullTextIndex_14495_" + i, "{a:'text'}", false,
                    false );
            FullTextDBUtils.insertData( cl, 100000 );
            cappedClNames.add( FullTextDBUtils.getCappedName( cl,
                    "fullTextIndex_14495_" + i ) );
            esIndexNames.add( FullTextDBUtils.getESIndexName( cl,
                    "fullTextIndex_14495_" + i ) );
        }
    }

    @Test
    public void Test() throws Exception {
        NodeWrapper node = groupMgr.getGroupByName( groupName ).getSlave();
        FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask( node, 1 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        taskMgr.addTask( new DropClTask() );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );

        // 此處會有索引殘留，同14494
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedClNames ) );
    }

    @AfterClass
    public void tearDown() {
        sdb.close();
    }

    private class DropClTask extends OperateTask {
        private Sequoiadb db = null;
        private CollectionSpace cs = null;

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = db.getCollectionSpace( csName );

            for ( int i = 0; i < 10; i++ ) {
                FullTextDBUtils.dropCollection( cs, "cl_14495_" + i );
            }
            db.close();
        }
    }
}
