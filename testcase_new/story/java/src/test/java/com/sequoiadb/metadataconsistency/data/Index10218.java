package com.sequoiadb.metadataconsistency.data;

import java.util.Random;

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
 * TestLink: seqDB-10218: concurrency[dropIndex, dropCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Index10218 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10218";
    private String clName = "cl10218";
    private String idxName = "idx";

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

        sdb.createCollectionSpace( csName ).createCollection( clName );
        MetaDataUtils.insertData( sdb, csName, clName );
        createIndex( sdb );
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
        te.addWorker( new CreateIndex() );
        te.addWorker( new DropCS() );
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, clName );
        MetaDataUtils.checkCLResult( csName, clName );
        MetaDataUtils.checkCSOfCatalog( csName );
    }

    private class CreateIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection clDB = db.getCollectionSpace( csName )
                        .getCollection( clName );

                String name = idxName;
                Random i = new Random();
                clDB.dropIndex( name + i.nextInt( 42 ) );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -47 // -47:Index name does not exist
                        && eCode != -248 // -248:Dropping the collection space
                                         // is in progress
                        && eCode != -23 && eCode != -34 && eCode != -147) {
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

    public void createIndex( Sequoiadb sdb ) {
        for ( int i = 0; i < 42; i++ ) {
            BSONObject opt = new BasicBSONObject();
            opt.put( "a" + i, 1 );
            String name = idxName;
            sdb.getCollectionSpace( csName ).getCollection( clName )
                    .createIndex( name + i, opt, false, false );

        }
    }

}
