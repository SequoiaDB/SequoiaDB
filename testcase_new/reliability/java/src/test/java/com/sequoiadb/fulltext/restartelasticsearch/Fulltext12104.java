package com.sequoiadb.fulltext.restartelasticsearch;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.RestartElasticSearch;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-12104:数据操作时ES正常重启
 * @author yinzhen
 * @date 2019-8-9
 */
public class Fulltext12104 extends SdbTestBase {
    private String clName = "cl12104";
    private String fulltextName = "idx12104";
    private Sequoiadb sdb;
    private String groupName;
    private GroupMgr groupMgr;

    @BeforeClass()
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
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName, ( BSONObject ) JSON
                        .parse( "{'Group':'" + groupName + "'}" ) );
        cl.createIndex( fulltextName, "{'a':'text', 'b':'text', 'c':'text'}",
                false, false );
        FullTextDBUtils.insertData( cl, 10000 );
    }

    @Test
    public void test() throws Exception {
        TaskMgr taskMgr = new TaskMgr();
        FaultMakeTask task = RestartElasticSearch
                .geFaultMakeTask( SdbTestBase.esHostName, 1 );
        taskMgr.addTask( task );
        taskMgr.addTask( new InsertData() );
        taskMgr.addTask( new UpdateData() );
        taskMgr.addTask( new DeleteData() );
        taskMgr.addTask( new QueryData() );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );

        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        FullTextUtils.isIndexCreated( cl, fulltextName, 20000 );
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cs.dropCollection( clName );
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
            } catch ( BaseException e ) {
                if ( -79 != e.getErrorCode() && -16 != e.getErrorCode() ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }
}
