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

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * TestLink: seqDB-10164: concurrency[drop cs, alter domain]
 * 
 * @author xiaoni huang init
 * @Date 2016.9.26
 */

public class CS10164 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10164";
    private String csName = "cs10164";
    private Random random = new Random();
    private int number = 20;

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
        createDomain( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            // clear env
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
        ThreadExecutor te = new ThreadExecutor();
        for ( int i = 0; i < 10; i++ ) {
            te.addWorker( new DropCS() );
            te.addWorker( new AlterDomain() );
        }
        te.run();

        // check results
        MetaDataUtils.checkDomainOfCatalog( domainName );
        MetaDataUtils.checkCSOfCatalog( csName );
    }

    private class DropCS extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                String tmpCSName = csName + "_" + random.nextInt( number );
                db.dropCollectionSpace( tmpCSName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -34 && eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class AlterDomain extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject opt = new BasicBSONObject();
                opt.put( "Groups", dataGroups.get( 0 ).split( "," ) );
                opt.put( "AutoSplit", false );
                db.getDomain( domainName ).alterDomain( opt );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 ) {
                    throw e;
                }
            }
        }
    }

    public void createDomain( Sequoiadb sdb ) {
        BSONObject opt = new BasicBSONObject();
        if ( !sdb.isDomainExist( domainName ) ) {
            opt.put( "Groups", dataGroups );
            opt.put( "AutoSplit", true );
            sdb.createDomain( domainName, opt );
        }
    }

    public void createCS( Sequoiadb sdb ) {
        for ( int i = 0; i < number; i++ ) {
            BSONObject opt = new BasicBSONObject();
            opt.put( "Domain", domainName );
            sdb.createCollectionSpace( csName + i, opt );
        }
    }

}