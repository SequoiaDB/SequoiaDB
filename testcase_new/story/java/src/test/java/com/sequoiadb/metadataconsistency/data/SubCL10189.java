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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10189: concurrency[attachCL, drop mainCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.14
 */

public class SubCL10189 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10189";
    private String clName = "cl10189";
    private String mCSName = csName + "_m";
    private String sCSName = csName + "_s";
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

        sdb.createCollectionSpace( mCSName );
        sdb.createCollectionSpace( sCSName );
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
        ThreadExecutor te = new ThreadExecutor( 300000 );
        te.addWorker( new AttachCL() );
        te.addWorker( new DropMainCS() );
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class AttachCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject options = new BasicBSONObject();
                BSONObject lowBoundObj = new BasicBSONObject();
                BSONObject upBoundObj = new BasicBSONObject();
                lowBoundObj.put( "a", 1 );
                upBoundObj.put( "a", 100 );
                options.put( "LowBound", lowBoundObj );
                options.put( "UpBound", upBoundObj );
                CollectionSpace csDB = db.getCollectionSpace( mCSName );
                if ( csDB.isCollectionExist( mCLName ) ) {
                    csDB.getCollection( mCLName ).attachCollection(
                            sCSName + "." + sCLName, options );

                    MetaDataUtils.insertData( db, csName, mCLName );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -34 && eCode != -147
                        && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class DropMainCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( mCSName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    public void createMainCL( Sequoiadb sdb ) {
        CollectionSpace csDB = sdb.getCollectionSpace( mCSName );

        BSONObject mOpt = new BasicBSONObject();
        BSONObject mSubObj = new BasicBSONObject();
        mSubObj.put( "a", 1 );
        mOpt.put( "ShardingKey", mSubObj );
        mOpt.put( "ReplSize", 0 );
        mOpt.put( "IsMainCL", true );
        csDB.createCollection( mCLName, mOpt );
    }

    public void createSubCL( Sequoiadb sdb ) {
        CollectionSpace csDB = sdb.getCollectionSpace( sCSName );

        BSONObject sOpt = new BasicBSONObject();
        BSONObject sSubObj = new BasicBSONObject();
        sSubObj.put( "a", 1 );
        sOpt.put( "ShardingKey", sSubObj );
        sOpt.put( "ReplSize", 0 );
        csDB.createCollection( sCLName, sOpt );
    }

}