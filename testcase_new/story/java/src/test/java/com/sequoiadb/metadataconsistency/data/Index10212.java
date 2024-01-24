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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10212: concurrency[createIndex, dropCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class Index10212 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10212";
    private String clName = "cl10212";
    private String idxName = "idx";

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

        CollectionSpace csDB = sdb.createCollectionSpace( csName );
        csDB.createCollection( clName );
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
        te.addWorker( new CreateIndex() );
        te.addWorker( new DropCL() );
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, clName );
    }

    private class CreateIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );

                String name = idxName;
                Random i = new Random();
                BSONObject opt = new BasicBSONObject();
                opt.put( "a" + i.nextInt( 10000 ), 1 );
                DBCollection clDB = csDB.getCollection( clName );
                if ( clDB != null ) {
                    clDB.createIndex( name + i.nextInt( 100 ), opt, false,
                            false );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -247 // -247:Redefine index
                        && eCode != -46 // -46:Duplicate index name
                        && eCode != -23 && eCode != -34 // -34:because is only
                                                        // one CL in CS, delete
                                                        // the CS data file when
                                                        // deleting the last CL,
                                                        // so exception -34 when
                                                        // creatIndex
                        && eCode != -108 && eCode != -147 && eCode != -190
                        && eCode != -243 ) {
                    throw e;
                }
            }
        }
    }

    private class DropCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( csName ).dropCollection( clName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

}