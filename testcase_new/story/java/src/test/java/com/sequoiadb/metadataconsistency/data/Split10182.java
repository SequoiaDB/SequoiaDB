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
 * TestLink: seqDB-10182: concurrency[split]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Split10182 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private static ArrayList< String > groupNames = null;
    private String csName = "cs10182";
    private String clName = "cl10182";
    private Random random = new Random();
    private int msec = 500;

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
        MetaDataUtils.clearCS( sdb, csName );

        groupNames = MetaDataUtils.getDataGroupNames( sdb );

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
        ThreadExecutor te = new ThreadExecutor();
        for ( int i = 0; i < 2; i++ ) {
            te.addWorker( new Split() );
        }
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class Split extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                BSONObject strCond = new BasicBSONObject();
                BSONObject endCond = new BasicBSONObject();
                Random i = new Random();
                int bound = i.nextInt( 10000 );
                strCond.put( "a", bound );
                endCond.put( "a", bound + 100 );
                MetaDataUtils.sleep( random.nextInt( msec ) );
                clDB.split( groupNames.get( 0 ), groupNames.get( 1 ), strCond,
                        endCond );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode(); // -176:SDB_CLS_BAD_SPLIT_KEY,split
                // bound exist
                if ( eCode != -175 && eCode != -147 && eCode != -176
                        && eCode != -190 ) { // -175:The
                    // mutex
                    // task
                    // already
                    // exist
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