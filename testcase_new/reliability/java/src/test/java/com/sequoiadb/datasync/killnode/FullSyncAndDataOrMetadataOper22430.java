package com.sequoiadb.datasync.killnode;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22430:全量同步过程中数据或元数据操作
 * @Author HuangXiaoNi 2020.7.17
 */

public class FullSyncAndDataOrMetadataOper22430 extends SdbTestBase {
    private boolean runSuccess = false;
    private GroupMgr groupMgr;
    private Sequoiadb sdb = null;
    private String groupName;
    private String[] csNames = { "cs22430_0", "cs22430_1", "cs22430_2" };
    private String clNameBase = "cl22430_";
    private String clNameForIdx = clNameBase + 2;
    private ArrayList< BSONObject > insertor = new ArrayList< BSONObject >();
    private int recsNum = 100000;

    @BeforeClass
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip standalone." );
        }

        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );

        // ready data
        for ( int i = 0; i < recsNum; i++ ) {
            insertor.add( new BasicBSONObject( "id", i ).append( "a", i ) );
        }

        // create CS/CL, and insert records
        for ( int i = 0; i < csNames.length; i++ ) {
            if ( sdb.isCollectionSpaceExist( csNames[ i ] ) ) {
                sdb.dropCollectionSpace( csNames[ i ] );
            }
            if ( i == 0 ) {
                // CS0 contains 3 CLs, use for
                // insert/truncate/(createIndex/dropIndex)
                CollectionSpace cs = sdb.createCollectionSpace( csNames[ i ] );
                for ( int j = 0; j < 3; j++ ) {
                    DBCollection cl = cs.createCollection( clNameBase + j,
                            new BasicBSONObject( "Group", groupName ) );
                    cl.insert( insertor );
                }
            } else if ( i == 1 ) {
                // CS1 use for renameCS, CS2.CL1 use for renameCL
                sdb.createCollectionSpace( csNames[ i ] ).createCollection(
                        clNameBase + 1,
                        new BasicBSONObject( "Group", groupName ) );
            }
        }
    }

    @Test
    private void test() throws Exception {
        TaskMgr taskMgr = new TaskMgr();
        GroupWrapper group = groupMgr.getGroupByName( groupName );
        NodeWrapper node = group.getSlave();
        taskMgr.addTask( KillNode.getFaultMakeTask( node, 60 ) );
        taskMgr.addTask( new Insert() );
        taskMgr.addTask( new Truncate() );
        taskMgr.addTask( new Index() );
        taskMgr.addTask( new RenameCSAndCL() );
        taskMgr.execute();
        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        // check renameCS/CL
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection cl;
            if ( db.isCollectionSpaceExist( csNames[ 1 ] ) ) {
                cl = db.getCollectionSpace( csNames[ 1 ] )
                        .getCollection( clNameBase + 1 );

            } else {
                cl = db.getCollectionSpace( csNames[ 2 ] )
                        .getCollection( clNameBase + 2 );
            }
            cl.insert( new BasicBSONObject( "a", 1 ) );
            Assert.assertEquals( cl.getCount(), 1 );
        }
        // check LSN
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        // check data
        Assert.assertEquals( group.checkInspect( 60 ), true );
        // check index
        CommLib commlib = new CommLib();
        commlib.checkIndex( sdb, csNames[ 0 ], clNameForIdx );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            try {
                for ( int i = 0; i < csNames.length; i++ ) {
                    if ( sdb.isCollectionSpaceExist( csNames[ i ] ) ) {
                        sdb.dropCollectionSpace( csNames[ i ] );
                    }
                }
            } finally {
                sdb.close();
            }
        }
    }

    private class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csNames[ 0 ] )
                        .getCollection( clNameBase + 0 );

                for ( BSONObject obj: insertor ){
                    obj.removeField("_id") ;
                }
                cl.insert( insertor );
            }
        }
    }

    private class Truncate extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csNames[ 0 ] )
                        .getCollection( clNameBase + 1 );
                cl.truncate();
            }
        }
    }

    private class Index extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csNames[ 0 ] )
                        .getCollection( clNameBase + 2 );
                for ( int i = 0; i < 10; i++ ) {
                    String indexName = "idx";
                    cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                            false, false );
                    cl.dropIndex( indexName );
                }
            }
        }
    }

    private class RenameCSAndCL extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                String csName1 = csNames[ 1 ];
                String csName2 = csNames[ 2 ];
                String clName1 = clNameBase + 1;
                String clName2 = clNameBase + 2;
                for ( int i = 0; i < 10; i++ ) {
                    // renameCS and renameCL
                    db.renameCollectionSpace( csName1, csName2 );
                    CollectionSpace cs1 = db.getCollectionSpace( csName2 );
                    cs1.renameCollection( clName1, clName2 );
                    // recover
                    db.renameCollectionSpace( csName2, csName1 );
                    CollectionSpace cs2 = db.getCollectionSpace( csName1 );
                    cs2.renameCollection( clName2, clName1 );
                }
            }
        }
    }
}
