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
 * TestLink: seqDB-10161: concurrency[create domain, alter domain, drop domain]
 * 
 * @author xiaoni huang init
 * @Date 2016.9.20
 */

public class Domain10161 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10161";
    private Random random = new Random();
    private int number = 20;

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or group number or node number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb )
                || MetaDataUtils.oneCataNode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or only one group or one node, "
                            + "skip the testCase." );
        }
        MetaDataUtils.clearDomain( sdb, domainName );
        dataGroups = MetaDataUtils.getDataGroupNames( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
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
        for ( int i = 0; i < 3; i++ ) {
            te.addWorker( new CreateDomain() );
            te.addWorker( new AlterDomain() );
            te.addWorker( new DropDomain() );
        }
        te.run();

        // check results
        MetaDataUtils.checkDomainOfCatalog( domainName );
    }

    private class CreateDomain extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject opt = new BasicBSONObject();
                opt.put( "Groups", dataGroups );
                opt.put( "AutoSplit", false );
                for ( int i = 0; i < 10; i++ ) {
                    db.createDomain(
                            domainName + "_" + random.nextInt( number ), opt );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -215 ) { // -215:Domain already exists
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
                int drSize = random.nextInt( dataGroups.size() );
                opt.put( "Groups", dataGroups.get( drSize ).split( "," ) );
                opt.put( "AutoSplit", true );
                for ( int i = 0; i < 10; i++ ) {
                    db.getDomain( domainName + "_" + random.nextInt( number ) )
                            .alterDomain( opt );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -214 ) { // -214:Domain does not exist
                    throw e;
                }
            }
        }
    }

    private class DropDomain extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < 5; i++ ) {
                    db.dropDomain(
                            domainName + "_" + random.nextInt( number ) );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -214 ) { // -214:Domain does not exist
                    throw e;
                }
            }
        }
    }

}
