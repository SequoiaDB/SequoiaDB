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
 * TestLink: seqDB-10207: concurrency[createIdIndex, drop CS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class IdIndex10207 extends SdbTestBase {

    private static Sequoiadb sdb = null;
    private String csName = "cs10207";
    private String clName = "cl10207";

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
        te.addWorker( new CreadeIdIndex() );
        te.addWorker( new DropCS() );
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, clName );
        MetaDataUtils.checkCSOfCatalog( csName );
    }

    private class CreadeIdIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );
                if ( csDB != null ) {
                    DBCollection clDB = csDB.getCollection( clName );

                    BSONObject opt = new BasicBSONObject();
                    opt.put( "SortBufferSize", 128 );
                    if ( clDB != null ) {
                        clDB.createIdIndex( opt );
                    }
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -34 && eCode != -147
                        && eCode != -248 && eCode != -190 ) {
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
                if ( eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    public void createCL( String csName ) {
        BSONObject opt = new BasicBSONObject();
        opt.put( "AutoIndexId", false );
        sdb.getCollectionSpace( csName ).createCollection( clName, opt );
    }

}