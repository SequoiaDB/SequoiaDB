package com.sequoiadb.metadataconsistency.data;

import java.util.Random;

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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10188:attachCL过程中删除子表
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class SubCL10188 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10188";
    private String clName = "cl10188";
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
        te.addWorker( new DropSubCL() );
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
                lowBoundObj.put( "a", 0 );
                upBoundObj.put( "a", 200 );
                options.put( "LowBound", lowBoundObj );
                options.put( "UpBound", upBoundObj );
                CollectionSpace csDB = db.getCollectionSpace( csName );
                if ( csDB.isCollectionExist( mCLName ) ) {
                    DBCollection clDB = csDB.getCollection( mCLName );
                    clDB.attachCollection( csName + "." + sCLName, options );

                    MetaDataUtils.insertData( db, csName, mCLName );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -34 && eCode != -135
                        && eCode != -147 ) {
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
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
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

}