package com.sequoiadb.fulltext.vote;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-12106:数据操作时连续切主
 * @Author Zhao xiaoni
 * @Date 2019-10-29
 */
public class Vote12106 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs12106";
    private String clName = "cl12106";
    private String indexName = "index12106";
    private DBCollection cl = null;
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private String cappedName = null;
    private String esIndexName = null;

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
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( !FullTextUtils.checkAdapter() ) {
            throw new SkipException( "Check adapter failed" );
        }

        cl = sdb.createCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cl.createIndex( indexName, "{a:'text',b:'text',c:'text',d:'text'}",
                false, false );
        FullTextDBUtils.insertData( cl, 10000 );
        cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );
    }

    @Test
    public void Test() throws Exception {

        TaskMgr taskMgr = new TaskMgr();
        taskMgr.addTask( new InsertTask() );
        taskMgr.addTask( new UpdateTask() );
        taskMgr.addTask( new DeleteTask() );
        taskMgr.addTask( new QueryTask() );
        taskMgr.addTask( new ReelectTask() );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
        Assert.assertTrue( FullTextUtils.checkAdapter() );
        FullTextUtils.isIndexCreated( cl, indexName, ( int ) cl.getCount() );
    }

    private class InsertTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            try {
                for ( int i = 0; i < 10; i++ ) {
                    FullTextDBUtils.insertData( cl, 10000 );
                }
            } finally {
                db.close();
            }
        }
    }

    private class UpdateTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            try {
                for ( int i = 0; i < 10; i++ ) {
                    cl.update( null,
                            "{$set:{'c':'test_update_data_" + i + "'}}", null );
                }
            } finally {
                db.close();
            }
        }
    }

    private class DeleteTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            try {
                for ( int i = 0; i < 10; i++ ) {
                    cl.delete( "{'recordId':" + ( 1000 + i ) + "}" );
                }
            } finally {
                db.close();
            }
        }
    }

    private class QueryTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            try {
                for ( int i = 0; i < 10; i++ ) {
                    DBCursor cursor = cl.query(
                            "{'': {'$Text': {'query': {'match_all': {}}}}}", "",
                            "", "" );
                    while ( cursor.hasNext() ) {
                        cursor.getNext();
                    }
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -6 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class ReelectTask extends OperateTask {
        private Sequoiadb db = null;

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            try {
                for ( int i = 0; i < 10; i++ ) {
                    db.getReplicaGroup( groupName ).reelect(
                            ( BSONObject ) JSON.parse( "{Seconds: 60}" ) );
                }
            } finally {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace( csName );
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
