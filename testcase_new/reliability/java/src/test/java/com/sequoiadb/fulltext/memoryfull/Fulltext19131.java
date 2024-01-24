package com.sequoiadb.fulltext.memoryfull;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.faultmodule.fault.FaultName;
import com.sequoiadb.faultmodule.task.FaultTask;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * 
 * @description seqDB-19131:增删改全文检索记录时主节点适配内存不足
 * @author yinzhen
 * @date 2019年9月5日
 */
public class Fulltext19131 extends SdbTestBase {
    private String clName = "cl19131";
    private String fulltextName = "idx19131";
    private String groupName;
    private Sequoiadb sdb;
    private GroupMgr groupMgr;
    private DBCollection cl;

    @BeforeClass()
    public void setUp() throws Exception {
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

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cl = sdb.getCollectionSpace( SdbTestBase.csName ).createCollection(
                clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cl.createIndex( fulltextName, "{'a':'text', 'b':'text', 'c':'text'}",
                false, false );
        FullTextDBUtils.insertData( cl, 10000 );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fulltextName, 10000 ) );
    }

    @Test
    public void test() throws Exception {
        Node node = sdb.getReplicaGroup( groupName ).getMaster();
        FaultTask task = FaultTask.getFault( FaultName.MEMORYLIMIT );
        try {
            String svcName = String.valueOf( node.getPort() );
            svcName = svcName.substring( 0, svcName.length() - 1 ) + "7";
            task.make( node.getHostName(), svcName, "root",
                    SdbTestBase.rootPwd );

            TaskMgr taskMgr = new TaskMgr();
            taskMgr.addTask( new InsertData() );
            taskMgr.addTask( new UpdateData() );
            taskMgr.addTask( new DeleteData() );
            taskMgr.addTask( new QueryData() );
            taskMgr.execute();

            Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        } finally {
            task.restore();
        }

        FullTextDBUtils.insertData( cl, 1000 );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fulltextName,
                ( int ) cl.getCount() ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    private class InsertData extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                for ( int i = 0; i < 10; i++ ) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    FullTextDBUtils.insertData( cl, 10000 );
                }
            } finally {
                db.close();
            }
        }
    }

    private class UpdateData extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                for ( int i = 0; i < 10; i++ ) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    cl.update( null,
                            "{$set:{'c':'test_update_data_" + i + "'}}", null );
                }
            } finally {
                db.close();
            }
        }
    }

    private class DeleteData extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                for ( int i = 0; i < 10; i++ ) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    cl.delete( "{'recordId':" + ( 1000 + i ) + "}" );
                }
            } finally {
                db.close();
            }
        }
    }

    private class QueryData extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                for ( int i = 0; i < 20; i++ ) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    DBCursor cursor = cl.query(
                            "{'': {'$Text': {'query': {'match_all': {}}}}}", "",
                            "", "" );
                    while ( cursor.hasNext() ) {
                        cursor.getNext();
                    }
                }
            } finally {
                db.close();
            }
        }
    }
}
