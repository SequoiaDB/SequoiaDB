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
 * TestLink: seqDB-10168: concurrency[drop cs of subCL, drop mainCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CS10168 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10168";
    private String clName = "cl10168";
    private String mCSName = csName + "_m";
    private String sCSName = csName + "_s";
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
        ThreadExecutor te = new ThreadExecutor( 300000 );
        te.addWorker( new DropMainCL() );
        te.addWorker( new DropSubCS() );
        te.run();

        // check results
        MetaDataUtils.checkCSOfCatalog( csName );
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class DropSubCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( sCSName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190
                        && e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

    private class DropMainCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( mCSName ).dropCollection( mCLName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190
                        && e.getErrorCode() != -23 ) {
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
        sdb.getCollectionSpace( mCSName ).createCollection( mCLName, opt );
    }

    public void createSubCL( Sequoiadb sdb ) {
        BSONObject opt = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "a", 1 );
        opt.put( "ShardingKey", subObj );
        opt.put( "ReplSize", 0 );
        sdb.getCollectionSpace( sCSName ).createCollection( sCLName, opt );
    }

    public void attachCL( Sequoiadb sdb ) {
        DBCollection clDB = sdb.getCollectionSpace( mCSName )
                .getCollection( mCLName );

        BSONObject opt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        BSONObject upBound = new BasicBSONObject();
        lowBound.put( "a", 0 );
        upBound.put( "a", 100 );
        opt.put( "LowBound", lowBound );
        opt.put( "UpBound", upBound );
        clDB.attachCollection( sCSName + "." + sCLName, opt );
    }

}