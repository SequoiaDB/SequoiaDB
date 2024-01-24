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

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10192: concurrency[split(mainCL/subCL), dropCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Split10192 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > groupNames = null;
    private String csName = "cs10192";
    private String clName = "cl10192";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";

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
        createMainCL( sdb );
        createSubCL( sdb );
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
        te.addWorker( new Split() );
        te.addWorker( new AttachCL() );
        te.addWorker( new DetachCL() );
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
                        .getCollection( sCLName );
                BSONObject strCond = new BasicBSONObject();
                BSONObject endCond = new BasicBSONObject();
                Random i = new Random();
                int bound = i.nextInt( 3996 );
                strCond.put( "a", bound );
                endCond.put( "a", bound + 100 );
                clDB.split( groupNames.get( 0 ), groupNames.get( 1 ), strCond,
                        endCond );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -175 && eCode != -147 && eCode != -190 ) { // -175:The
                    // mutex
                    // task
                    // already
                    // exist
                    throw e;
                }
            }
        }
    }

    private class AttachCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                attachCL( db );

                try {
                    MetaDataUtils.insertData( db, csName, mCLName );
                } catch ( BaseException e ) {
                    int eCode = e.getErrorCode();
                    if ( eCode != -135 ) {
                        throw e;
                    }
                }

            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -235 ) {
                    throw e;
                }
            }
        }
    }

    private class DetachCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( mCLName );

                clDB.detachCollection( csName + "." + sCLName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -242 ) { // -242:Invalid collection partition
                    throw e;
                }
            }
        }
    }

    public void createMainCL( Sequoiadb sdb ) {
        BSONObject opt = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "a", 1 );
        opt.put( "ShardingKey", subObj );
        opt.put( "ReplSize", 0 );
        opt.put( "IsMainCL", true );
        sdb.getCollectionSpace( csName ).createCollection( mCLName, opt );
    }

    public void createSubCL( Sequoiadb sdb ) {
        BSONObject opt = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "a", 1 );
        opt.put( "ShardingKey", subObj );
        opt.put( "Group", groupNames.get( 0 ) );
        opt.put( "ReplSize", 0 );
        sdb.getCollectionSpace( csName ).createCollection( sCLName, opt );

    }

    public void attachCL( Sequoiadb sdb ) {
        BSONObject options = new BasicBSONObject();
        BSONObject lowBoundObj = new BasicBSONObject();
        BSONObject upBoundObj = new BasicBSONObject();
        lowBoundObj.put( "a", 0 );
        upBoundObj.put( "a", 100 );
        options.put( "LowBound", lowBoundObj );
        options.put( "UpBound", upBoundObj );
        DBCollection clDB = sdb.getCollectionSpace( csName )
                .getCollection( mCLName );
        clDB.attachCollection( csName + "." + sCLName, options );
    }

}