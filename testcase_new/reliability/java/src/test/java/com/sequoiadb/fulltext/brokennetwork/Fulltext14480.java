package com.sequoiadb.fulltext.brokennetwork;

import java.util.ArrayList;
import java.util.List;

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
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-14480:删除全文索引时ES与适配器断网
 * @author yinzhen
 * @date 2019-8-6
 */
public class Fulltext14480 extends SdbTestBase {
    private String clName = "cl14480";
    private String fulltextName = "idx14480";
    private Sequoiadb sdb;
    private GroupMgr groupMgr;
    private List< String > cappedESNames = new ArrayList<>();

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
        for ( int i = 0; i < 10; i++ ) {
            String clName = this.clName + "_" + i;
            DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .createCollection( clName );
            FullTextDBUtils.insertData( cl, 10000 );
            cl.createIndex( fulltextName,
                    "{'a':'text', 'b':'text', 'c':'text'}", false, false );
            String cappedClName = FullTextDBUtils.getCappedName( cl,
                    fulltextName );
            cappedESNames.add( cappedClName );
            String esIndexName = FullTextDBUtils.getESIndexName( cl,
                    fulltextName );
            cappedESNames.add( esIndexName );
        }
    }

    @Test
    public void test() throws Exception {
        TaskMgr taskMgr = new TaskMgr();
        FaultMakeTask task = BrokenNetwork.getFaultMakeTask( esHostName, 1, 3 );
        taskMgr.addTask( task );
        taskMgr.addTask( new InsertData() );
        taskMgr.addTask( new DropIndex() );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );

        for ( int i = 0; i < 10; i++ ) {
            String cappedName = cappedESNames.get( i * 2 );
            String esIndexName = cappedESNames.get( i * 2 + 1 );
            FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName );

            String clName = this.clName + "_" + i;
            DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            Assert.assertTrue( FullTextUtils.isCLConsistency( cl ) );
            Assert.assertTrue( FullTextUtils.isCLDataConsistency( cl ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            for ( int i = 0; i < 10; i++ ) {
                String clName = this.clName + "_" + i;
                cs.dropCollection( clName );
            }
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
                    String clName = Fulltext14480.this.clName + "_" + i;
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

    private class DropIndex extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                for ( int i = 0; i < 10; i++ ) {
                    String clName = Fulltext14480.this.clName + "_" + i;
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    cl.dropIndex( fulltextName );
                }
            } finally {
                db.close();
            }
        }
    }

}
