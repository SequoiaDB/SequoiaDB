package com.sequoiadb.metadataconsistency.data;

import java.util.Date;
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

public class CL10177 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String csName = "cs10177";
    private String clName = "cl10177";
    private Random random = new Random();
    private int number = 20;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or node number
        if ( CommLib.isStandAlone( sdb ) || MetaDataUtils.oneCataNode( sdb )
                || MetaDataUtils.oneDataNode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or one node, skip the testCase." );
        }
        MetaDataUtils.clearCS( sdb, csName );

        sdb.createCollectionSpace( csName );
        createCL();
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
            te.addWorker( new AlterCL() );
        }
        te.run();

        // check results
        System.out.println( new Date() + " " + this.getClass().getName()
                + " begin check results " );
        MetaDataUtils.checkCLResult( csName, clName );
        System.out.println( new Date() + " " + this.getClass().getName()
                + " end check results " );
    }

    private class AlterCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            int rep = random.nextInt( 7 );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", rep );
                csDB.getCollection( clName + "_" + random.nextInt( number ) )
                        .alterCollection( opt );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    System.out.println( "ReplSize:" + rep );
                    throw e;
                }
            }
        }
    }

    public void createCL() {
        CollectionSpace csDB = sdb.getCollectionSpace( csName );

        BSONObject opt = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "a", 1 );
        opt.put( "ShardingType", "hash" );
        opt.put( "ShardingKey", subObj );
        opt.put( "ReplSize", 0 );
        opt.put( "AutoSplit", true );
        for ( int i = 0; i < number; i++ ) {
            String tmpCLName = clName + "_" + i;
            csDB.createCollection( tmpCLName, opt );
            MetaDataUtils.insertData( sdb, csName, tmpCLName );
        }
    }

}