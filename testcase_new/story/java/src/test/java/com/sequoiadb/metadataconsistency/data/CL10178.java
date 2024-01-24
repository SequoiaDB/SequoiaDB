package com.sequoiadb.metadataconsistency.data;

import java.util.ArrayList;

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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10178:修改cl过程中删除该cl
 * 
 * @author xiaoni huang init
 * @Date 2016.10.11
 */

public class CL10178 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String domainName = "dm10178";
    private String csName = "cs10178";
    private String clName = "cl10178";

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
        createCL();
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
        for ( int i = 0; i < 10; i++ ) {
            te.addWorker( new AlterCL() );
            te.addWorker( new DropCL() );
        }
        te.run();

        // check results
        MetaDataUtils.checkCLResult( csName, clName );
    }

    private class AlterCL extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace csDB = db.getCollectionSpace( csName );

                BSONObject opt = new BasicBSONObject();
                opt.put( "ReplSize", 2 );
                DBCollection clDB = csDB.getCollection( clName );
                if ( clDB != null ) {
                    clDB.alterCollection( opt );
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -23 && eCode != -108
                        && eCode != -190 ) {
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

                csDB.dropCollection( clName );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -147 && eCode != -190 && eCode != -23 ) {
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

    public void createCL() {
        CollectionSpace csDB = sdb.getCollectionSpace( csName );

        BSONObject opt = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "a", 1 );
        opt.put( "ShardingType", "hash" );
        opt.put( "ShardingKey", subObj );
        opt.put( "ReplSize", 1 );
        opt.put( "AutoSplit", true );
        csDB.createCollection( clName, opt );
    }

}