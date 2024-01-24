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
 * TestLink: seqDB-10210: concurrency[dropIdIndex, dropCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.17
 */

public class IdIndex10210 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10210";
    private String clName = "cl10210";

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
        te.addWorker( new DropCS() );
        te.run();

        // check results
        MetaDataUtils.checkIndex( csName, clName );
        MetaDataUtils.checkCSOfCatalog( csName );
    }

    private class DropIdIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                if ( db.isCollectionSpaceExist( csName ) ) {
                    CollectionSpace csDB = db.getCollectionSpace( csName );
                    if ( csDB.isCollectionExist( clName ) ) {
                        DBCollection clDB = csDB.getCollection( clName );
                        if ( clDB != null ) {
                            clDB.dropIdIndex();
                        }
                    }
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -248 && eCode != -23 && eCode != -34
                        && eCode != -147 && eCode != -190 ) {
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
                if ( e.getErrorCode() != -34 && e.getErrorCode() != -147
                        && e.getErrorCode() != -190 ) {
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