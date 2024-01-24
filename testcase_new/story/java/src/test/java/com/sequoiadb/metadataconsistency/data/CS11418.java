package com.sequoiadb.metadataconsistency.data;

import com.sequoiadb.exception.SDBError;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * TestLink: seqDB-11418:并发删除不同CS
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class CS11418 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs11418";
    private String clName = "cl11418";
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
        attachCL( sdb );
        MetaDataUtils.insertData( sdb, mCSName, mCLName );
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
        for ( int i = 0; i < 10; i++ ) {
            te.addWorker( new DropMainCS() );
            te.addWorker( new DropSubCS() );
        }
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class DropMainCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( mCSName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                        && eCode != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && eCode != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                        && eCode != SDBError.SDB_DMS_CS_DELETING
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class DropSubCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( sCSName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                        && eCode != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && eCode != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                        && eCode != SDBError.SDB_DMS_CS_DELETING
                                .getErrorCode() ) {
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

    public void attachCL( Sequoiadb sdb ) {
        DBCollection clDB = sdb.getCollectionSpace( mCSName )
                .getCollection( mCLName );

        BSONObject options = new BasicBSONObject();
        BSONObject lowBoundObj = new BasicBSONObject();
        BSONObject upBoundObj = new BasicBSONObject();
        lowBoundObj.put( "a", 0 );
        upBoundObj.put( "a", 100 );
        options.put( "LowBound", lowBoundObj );
        options.put( "UpBound", upBoundObj );
        clDB.attachCollection( sCSName + "." + sCLName, options );
    }

}