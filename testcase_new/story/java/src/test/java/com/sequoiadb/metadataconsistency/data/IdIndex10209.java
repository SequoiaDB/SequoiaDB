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
 * TestLink: seqDB-10209: concurrency[dropIdIndex, alterCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.20
 */

public class IdIndex10209 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10209";
    private String clName = "cl10209";

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

        sdb.createCollectionSpace( csName );
        createCL( csName );
        MetaDataUtils.insertData( sdb, csName, clName );
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
        te.addWorker( new DropIdIndex() );
        te.addWorker( new AlterCL() );
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, clName );
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class DropIdIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                clDB.dropIdIndex();
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -108 && eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class AlterCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );
                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", 1 );
                csDB.getCollection( clName ).alterCollection( opt );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    public void createCL( String csName ) {
        BSONObject opt = new BasicBSONObject();
        opt.put( "AutoIndexId", true );
        sdb.getCollectionSpace( csName ).createCollection( clName, opt );
    }

}