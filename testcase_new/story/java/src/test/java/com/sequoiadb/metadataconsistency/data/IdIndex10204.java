package com.sequoiadb.metadataconsistency.data;

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
 * TestLink: seqDB-10204: concurrency[create IdIndex]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class IdIndex10204 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10204";
    private String clName = "cl10204";

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or node number
        if ( CommLib.isStandAlone( sdb ) || MetaDataUtils.oneDataNode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or one node, skip the testCase." );
        }
        MetaDataUtils.clearCS( sdb, csName );

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
        for ( int i = 0; i < 3; i++ ) {
            te.addWorker( new CreateIdIndex() );
        }
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, clName );
    }

    private class CreateIdIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "SortBufferSize", 128 );
                clDB.createIdIndex( opt );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -247 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    public void createCL( String csName ) {
        CollectionSpace csDB = sdb.createCollectionSpace( csName );

        BSONObject opt = new BasicBSONObject();
        opt.put( "AutoIndexId", false );
        csDB.createCollection( clName, opt );
    }
}