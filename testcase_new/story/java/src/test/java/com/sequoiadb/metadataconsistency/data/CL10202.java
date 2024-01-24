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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10202: concurrency[drop mainCL,dropSubCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CL10202 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10202";
    private String clName = "cl10202";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or node number
        if ( CommLib.isStandAlone( sdb ) || MetaDataUtils.oneCataNode( sdb )
                || MetaDataUtils.oneDataNode( sdb ) ) {
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
        ThreadExecutor te = new ThreadExecutor( 300000 );
        for ( int i = 0; i < 10; i++ ) {
            te.addWorker( new DropMainCL() );
            te.addWorker( new DropSubCL() );
        }
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class DropMainCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( csName ).dropCollection( mCLName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class DropSubCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );

                csDB.dropCollection( sCLName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    public void createMainCL( Sequoiadb sdb ) {
        CollectionSpace csDB = sdb.getCollectionSpace( csName );

        BSONObject mOpt = new BasicBSONObject();
        BSONObject mSubObj = new BasicBSONObject();
        mSubObj.put( "a", 1 );
        mOpt.put( "ShardingKey", mSubObj );
        mOpt.put( "ReplSize", 0 );
        mOpt.put( "IsMainCL", true );
        csDB.createCollection( mCLName, mOpt );
    }

    public void createSubCL( Sequoiadb sdb ) {
        CollectionSpace csDB = sdb.getCollectionSpace( csName );

        BSONObject sOpt = new BasicBSONObject();
        BSONObject sSubObj = new BasicBSONObject();
        sSubObj.put( "a", 1 );
        sOpt.put( "ShardingKey", sSubObj );
        sOpt.put( "ReplSize", 0 );
        csDB.createCollection( sCLName, sOpt );
    }

    public void attachCL( Sequoiadb sdb ) {
        CollectionSpace csDB = sdb.getCollectionSpace( csName );

        BSONObject opt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        BSONObject upBound = new BasicBSONObject();
        lowBound.put( "a", 0 );
        upBound.put( "a", 200 );
        opt.put( "LowBound", lowBound );
        opt.put( "UpBound", upBound );
        if ( csDB.isCollectionExist( sCLName ) ) {
            DBCollection clDB = csDB.getCollection( mCLName );
            clDB.attachCollection( csName + "." + sCLName, opt );
        }
    }

}