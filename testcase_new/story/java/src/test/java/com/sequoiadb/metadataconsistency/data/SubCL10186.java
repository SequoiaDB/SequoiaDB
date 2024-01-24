package com.sequoiadb.metadataconsistency.data;

import java.util.Date;
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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10186: concurrency[attachCL]
 * seqDB-29793:同时自动清理回收站中CL项目和所属CS项目(用例测试点在此用例可以覆盖)
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class SubCL10186 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10186";
    private String clName = "cl10186";
    private String mCLName = clName + "_m";
    private String sCLName = clName + "_s";
    private Random random = new Random();
    private int number = 20;

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
    }

    @AfterClass
    public void tearDown() {
        try {
            MetaDataUtils.clearCL( sdb, csName, clName );
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
        for ( int i = 0; i < 3; i++ ) {
            te.addWorker( new AttachCL() );
        }
        te.run();

        // check results
        System.out.println( new Date() + " " + this.getClass().getName()
                + " begin check results " );
        MetaDataUtils.checkCLResult( csName, clName );
        System.out.println( new Date() + " " + this.getClass().getName()
                + " end check results " );
    }

    private class AttachCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );

                BSONObject options = new BasicBSONObject();
                BSONObject lowBoundObj = new BasicBSONObject();
                BSONObject upBoundObj = new BasicBSONObject();
                int k = random.nextInt( 10000 );
                lowBoundObj.put( "a", k );
                upBoundObj.put( "a", 100 + k );
                options.put( "LowBound", lowBoundObj );
                options.put( "UpBound", upBoundObj );
                csDB.getCollection( mCLName + random.nextInt( number ) )
                        .attachCollection( csName + "." + sCLName
                                + random.nextInt( number ), options );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != SDBError.SDB_RELINK_SUB_CL.getErrorCode()
                        && eCode != SDBError.SDB_BOUND_CONFLICT
                                .getErrorCode() ) {
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
        for ( int i = 0; i < number; i++ ) {
            csDB.createCollection( mCLName + i, mOpt );
        }
    }

    public void createSubCL( Sequoiadb sdb ) {
        CollectionSpace csDB = sdb.getCollectionSpace( csName );

        BSONObject sOpt = new BasicBSONObject();
        BSONObject sSubObj = new BasicBSONObject();
        sSubObj.put( "a", 1 );
        sOpt.put( "ShardingKey", sSubObj );
        sOpt.put( "ReplSize", 0 );
        for ( int i = 0; i < number; i++ ) {
            csDB.createCollection( sCLName + i, sOpt );
        }
    }

}