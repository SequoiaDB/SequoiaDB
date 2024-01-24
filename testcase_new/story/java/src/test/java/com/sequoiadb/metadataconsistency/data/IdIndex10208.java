package com.sequoiadb.metadataconsistency.data;

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
 * TestLink: seqDB-10208: concurrency[dropIndex in mainCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.20
 */

public class IdIndex10208 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10208";
    private String clName = "cs10208";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";

    @BeforeClass
    public void setUp() {
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
        createIdIndex( sdb );
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
        ThreadExecutor te = new ThreadExecutor( 180000 );
        for ( int i = 0; i < 10; i++ ) {
            te.addWorker( new DropIndex() );
        }
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, sCLName );
    }

    private class DropIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( mCLName );
                clDB.dropIdIndex();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -247 && e.getErrorCode() != -147
                        && e.getErrorCode() != -190 ) {
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
        for ( int i = 0; i < 3; i++ ) {
            sdb.getCollectionSpace( csName ).createCollection( sCLName + i,
                    opt );
        }
    }

    public void attachCL( Sequoiadb sdb ) {
        DBCollection clDB = sdb.getCollectionSpace( csName )
                .getCollection( mCLName );

        BSONObject options = new BasicBSONObject();
        BSONObject lowBoundObj = new BasicBSONObject();
        BSONObject upBoundObj = new BasicBSONObject();
        for ( int i = 0; i < 3; i++ ) {
            int bound = i * 100;
            lowBoundObj.put( "a", bound );
            upBoundObj.put( "a", bound + 100 );
            options.put( "LowBound", lowBoundObj );
            options.put( "UpBound", upBoundObj );
            clDB.attachCollection( csName + "." + sCLName + i, options );
        }
    }

    public void createIdIndex( Sequoiadb sdb ) {
        DBCollection clDB = sdb.getCollectionSpace( csName )
                .getCollection( mCLName );
        BSONObject opt2 = new BasicBSONObject();
        opt2.put( "SortBufferSize", 128 );
        clDB.createIdIndex( opt2 );
    }
}