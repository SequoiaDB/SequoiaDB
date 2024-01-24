package com.sequoiadb.metadataconsistency.data;

import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-11417: concurrency[dropCS the same cs]
 * 
 * @author xiaoni huang init
 * @Date 2016.9.25
 */

public class CS11417 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs11417";

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or node number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or one node, skip the testCase." );
        }
        MetaDataUtils.clearCS( sdb, csName );
        this.createCS();
    }

    @AfterClass
    public void tearDown() {
        try {
            this.createCS();
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
        for ( int i = 0; i < 10; i++ ) {
            te.addWorker( new DropCS() );
        }
        te.run();

        // check results
        MetaDataUtils.checkCSOfCatalog( csName );
    }

    private class DropCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -34 && eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    private void createCS() throws BaseException {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            db.createCollectionSpace( csName );
        }
    }

}