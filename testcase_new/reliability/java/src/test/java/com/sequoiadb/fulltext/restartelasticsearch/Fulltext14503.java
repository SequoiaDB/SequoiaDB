package com.sequoiadb.fulltext.restartelasticsearch;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
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
 * @Description seqDB-14503:删除集合空间时ES正常重启
 * @author yinzhen
 * @date 2019-8-9
 */
public class Fulltext14503 extends SdbTestBase {
    private String clName = "cl14503";
    private String csName = "cs14503";
    private String fulltextName = "idx14503";
    private Sequoiadb sdb;
    private String groupName;
    private GroupMgr groupMgr;
    List< String > cappedESNames = new ArrayList<>();

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

        for ( int i = 0; i < 10; i++ ) {
            String csName = this.csName + "_" + i;
            String clName = this.clName + "_" + i;
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }

            DBCollection cl = sdb.createCollectionSpace( csName )
                    .createCollection( clName, ( BSONObject ) JSON
                            .parse( "{'Group':'" + groupName + "'}" ) );
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
        FaultMakeTask task = RestartElasticSearch
                .geFaultMakeTask( SdbTestBase.esHostName, 1 );
        taskMgr.addTask( task );
        taskMgr.addTask( new DropCS() );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        for ( int i = 0; i < 10; i++ ) {
            String cappedName = cappedESNames.get( i * 2 );
            String esIndexName = cappedESNames.get( i * 2 + 1 );
            FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName );

            String csName = this.csName + "_" + i;
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
        }
        Assert.assertTrue( FullTextUtils.checkAdapter() );
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < 10; i++ ) {
                String csName = this.csName + "_" + i;
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    FullTextDBUtils.dropCollectionSpace( sdb, csName );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class DropCS extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                for ( int i = 0; i < 10; i++ ) {
                    String csName = Fulltext14503.this.csName + "_" + i;
                    db.dropCollectionSpace( csName );
                }
            } catch ( BaseException e ) {
                if ( -147 != e.getErrorCode() ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

}