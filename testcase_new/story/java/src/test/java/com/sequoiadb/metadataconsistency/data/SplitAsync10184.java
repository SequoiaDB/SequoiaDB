package com.sequoiadb.metadataconsistency.data;

import java.util.ArrayList;
import java.util.Random;

import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10184: concurrency[splitAsync, dropCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class SplitAsync10184 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private static ArrayList< String > groupNames = null;
    private String csName = "cs10184_splitAsync";
    private String clName = "cl10184";
    private Random random = new Random();
    private int msec = 300;

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or group number or node number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb )
                || MetaDataUtils.oneCataNode( sdb )
                || MetaDataUtils.oneDataNode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or only one group or one node, "
                            + "skip the testCase." );
        }
        // get groupNames
        groupNames = MetaDataUtils.getDataGroupNames( sdb );

        MetaDataUtils.clearCS( sdb, csName );

        sdb.createCollectionSpace( csName );
        createCL( sdb, groupNames.get( 0 ) );
        MetaDataUtils.insertData( sdb, csName, clName );
    }

    @AfterClass
    public void tearDown() {
        try {
            MetaDataUtils.clearCS( sdb, csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor te = new ThreadExecutor( 300000 );
        te.addWorker( new SplitAsync() );
        te.addWorker( new DropCS() );
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class SplitAsync extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                if ( db.getCollectionSpace( csName )
                        .isCollectionExist( clName ) ) {
                    DBCollection clDB = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    BSONObject strCond = new BasicBSONObject();
                    BSONObject endCond = new BasicBSONObject();
                    strCond.put( "a", 0 );
                    endCond.put( "a", 50 );
                    long taskId = clDB.splitAsync( groupNames.get( 0 ),
                            groupNames.get( 1 ), strCond, endCond );
                    long[] taskIds = new long[ 1 ];
                    taskIds[ 0 ] = taskId;
                    db.waitTasks( taskIds );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -175 // -175:The mutex task already exist
                        && eCode != -147 && eCode != -23 && eCode != -34
                        && eCode != -190 && eCode != -243 ) {
                    throw e;
                }
            }
        }
    }

    private class DropCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    private void createCL( Sequoiadb sdb, String rgName ) {
        CollectionSpace csDB = sdb.getCollectionSpace( csName );
        BSONObject opt = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "a", 1 );
        opt.put( "ShardingType", "range" );
        opt.put( "ShardingKey", subObj );
        opt.put( "Group", rgName );
        opt.put( "ReplSize", 0 );
        csDB.createCollection( clName, opt );
    }

}