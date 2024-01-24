package com.sequoiadb.metadataconsistency.data;

import java.util.ArrayList;

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
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10184: concurrency[split, dropCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Split10184 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private static ArrayList< String > groupNames = null;
    private String csName = "cs10184_split";
    private String clName = "cl10184";

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
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        groupNames = MetaDataUtils.getDataGroupNames( sdb );

        sdb.createCollectionSpace( csName );
        createCL( sdb, groupNames.get( 0 ) );
        MetaDataUtils.insertData( sdb, csName, clName );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor te = new ThreadExecutor( 300000 );
        te.addWorker( new Split() );
        te.addWorker( new DropCS() );
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class Split extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );
                if ( csDB != null ) {
                    DBCollection clDB = csDB.getCollection( clName );
                    if ( clDB != null ) {
                        BSONObject strCond = new BasicBSONObject();
                        BSONObject endCond = new BasicBSONObject();
                        strCond.put( "a", 0 );
                        endCond.put( "a", 50 );
                        // System.out.println("split condition: " + strCond + ",
                        // " +endCond );
                        clDB.split( groupNames.get( 0 ), groupNames.get( 1 ),
                                strCond, endCond );
                    }
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != SDBError.SDB_CLS_MUTEX_TASK_EXIST.getErrorCode()
                        && eCode != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && eCode != SDBError.SDB_DMS_NOTEXIST.getErrorCode()
                        && eCode != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                        && eCode != SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                        && eCode != SDBError.SDB_TASK_HAS_CANCELED
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class DropCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && eCode != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
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