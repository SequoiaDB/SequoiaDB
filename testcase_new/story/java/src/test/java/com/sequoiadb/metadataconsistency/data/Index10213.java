package com.sequoiadb.metadataconsistency.data;

import java.util.Random;

import com.sequoiadb.exception.SDBError;
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
 * TestLink: seqDB-10213: concurrency[createIndex in mainCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.20
 */

public class Index10213 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10213";
    private String clName = "cl10213";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";
    private String idxName = "idx10213_";

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or node number
        if ( CommLib.isStandAlone( sdb ) || MetaDataUtils.oneDataNode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or one node, skip the testCase." );
        }
        MetaDataUtils.clearCS( sdb, csName );

        sdb.createCollectionSpace( csName );
        createMainCL( sdb );
        createSubCL( sdb );
        attachCL( sdb );
        MetaDataUtils.insertData( sdb, csName, mCLName );
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
        for ( int i = 0; i < 5; i++ ) {
            te.addWorker( new CreateIndex() );
        }
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, sCLName );
    }

    private class CreateIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( mCLName );

                String name = idxName;
                Random i = new Random();
                BSONObject opt = new BasicBSONObject();
                opt.put( "a" + i.nextInt( 10 ), 1 );
                clDB.createIndex( name + i.nextInt( 10 ), opt, false, false );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_IXM_REDEF.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_EXIST_COVERD_ONE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_EXIST
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_COVER_CREATING
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_SAME_NAME_CREATING
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_CREATING
                                .getErrorCode() ) {
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
        opt.put( "ReplSize", 0 );
        for ( int i = 0; i < 10; i++ ) {
            sdb.getCollectionSpace( csName ).createCollection( sCLName + i,
                    opt );
        }

    }

    public void attachCL( Sequoiadb sdb ) {
        BSONObject options = new BasicBSONObject();
        BSONObject lowBoundObj = new BasicBSONObject();
        BSONObject upBoundObj = new BasicBSONObject();
        for ( int i = 0; i < 10; i++ ) {
            int bound = i * 100;
            lowBoundObj.put( "a", bound );
            upBoundObj.put( "a", bound + 100 );
            options.put( "LowBound", lowBoundObj );
            options.put( "UpBound", upBoundObj );
            sdb.getCollectionSpace( csName ).getCollection( mCLName )
                    .attachCollection( csName + "." + sCLName + i, options );
        }
    }
}