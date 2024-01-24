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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10171: concurrency[createCL, alterCL, dropCL]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CL10171 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10171";
    private String clName = "cl10171";
    private Random random = new Random();
    private int number = 20;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or node number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or one node, skip the testCase." );
        }
        MetaDataUtils.clearCS( sdb, csName );
        sdb.createCollectionSpace( csName );
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
        te.addWorker( new CreateCL() );
        te.addWorker( new AlterCL() );
        te.addWorker( new DropCL() );
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class CreateCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );

                String tmpCLName = clName + "_" + random.nextInt( number );
                csDB.createCollection( tmpCLName );
                if ( csDB.isCollectionExist( tmpCLName ) ) {
                    MetaDataUtils.insertData( db, csName, tmpCLName );
                }

            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -22 && eCode != -147 && eCode != -190 ) {
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
                if ( csDB.isCollectionExist( clName ) ) {
                    csDB.getCollection(
                            clName + "_" + random.nextInt( number ) )
                            .alterCollection( opt );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -147 && eCode != -190 ) {
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
                CollectionSpace csDB = db.getCollectionSpace( csName );

                csDB.dropCollection( clName + "_" + random.nextInt( number ) );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -23 && eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

}
