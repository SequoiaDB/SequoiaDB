package com.sequoiadb.metadataconsistency.data;

import java.util.ArrayList;
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
 * TestLink: seqDB-10179: concurrency[alterCL, dropCS]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CL10179 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10179";
    private String csName = "cs10179";
    private String clName = "cl10179";

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
        MetaDataUtils.clearDomain( sdb, domainName );

        dataGroups = MetaDataUtils.getDataGroupNames( sdb );

        createDomain();
        createCS();
        sdb.getCollectionSpace( csName ).createCollection( clName );
        MetaDataUtils.insertData( sdb, csName, clName );
    }

    @AfterClass
    public void tearDown() {
        try {
            MetaDataUtils.clearCS( sdb, csName );
            MetaDataUtils.clearDomain( sdb, domainName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor te = new ThreadExecutor( 300000 );
        te.addWorker( new AlterCL() );
        te.addWorker( new DropCS() );
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class AlterCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {

                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", 7 );
                CollectionSpace csDB = db.getCollectionSpace( csName );
                if ( csDB != null ) {
                    csDB.getCollection( clName ).alterCollection( opt );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -34 && eCode != -23 && eCode != -147
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

    public void createDomain() {
        BSONObject opt = new BasicBSONObject();
        opt.put( "Groups", dataGroups );
        opt.put( "AutoSplit", true );
        sdb.createDomain( domainName, opt );
    }

    public void createCS() {
        BSONObject opt = new BasicBSONObject();
        opt.put( "Domain", domainName );
        sdb.createCollectionSpace( csName, opt );
    }

}