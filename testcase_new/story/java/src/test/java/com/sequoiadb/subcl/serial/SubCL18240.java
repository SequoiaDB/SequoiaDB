package com.sequoiadb.subcl.serial;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-18240:大量查询范围外的数据（性能）
 * @author huangxiaoni
 * @date 2019.3.21
 * @review
 */

public class SubCL18240 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb cata = null;
    private DBCollection mainCL;
    private final static String MAIN_CS_NAME = "main_cs_18240";
    private final static String SUB_CS_NAME = "sub_cs_18240";
    private final static String CL_NAME_BASE = "cl_";
    private final static int CL_NUM = 1000;
    private final static int RUN_TIMES = 1000; // run times for insert and query

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone, or only one group, skip the testCase." );
        }
        cata = sdb.getReplicaGroup( "SYSCatalogGroup" ).getMaster().connect();

        CollectionSpace mCS = sdb.createCollectionSpace( MAIN_CS_NAME );
        CollectionSpace sCS = sdb.createCollectionSpace( SUB_CS_NAME );
        this.readyCL( mCS, sCS );
    }

    @Test()
    private void test() throws Exception {
        // resetSnapshot before test
        cata.resetSnapshot();

        // insert and query
        BSONObject obj = new BasicBSONObject( "a", 200 );
        for ( int i = 0; i < RUN_TIMES; i++ ) {
            try {
                mainCL.insert( obj );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -135 );
            }
            DBCursor cursor = mainCL.query( obj, null, null, null );
            Assert.assertFalse( cursor.hasNext() );
        }

        // check results by catalog.snapshot(SDB_SNAP_DATABASE)
        int sdbSnapDataBase = 6;
        DBCursor cursor = cata.getSnapshot( sdbSnapDataBase, "", "", "" );
        BSONObject info = cursor.getNext();
        long tdr = ( long ) info.get( "TotalDataRead" );
        long tir = ( long ) info.get( "TotalIndexRead" );
        long expDiffValue = 20 * RUN_TIMES; // test value, error in existence,
                                            // ensure that the diff is not too
                                            // big
        long actDiffValue = Math.abs( tdr - tir );
        // System.out.println("TotalDataRead: " + tdr + ", TotalIndexRead: " +
        // tir + ", actDiffValue: " + actDiffValue);
        if ( actDiffValue > expDiffValue ) {
            Assert.fail( "Frequency table scanning"
                    + ", expect (TotalIndexRead - TotalDataRead) = "
                    + expDiffValue
                    + ", actual (TotalIndexRead - TotalDataRead) = "
                    + actDiffValue + ", TotalDataRead: " + tdr
                    + ", TotalIndexRead: " + tir );
        }

        // check resords
        Assert.assertEquals( mainCL.getCount(), 0 );
    }

    @AfterClass
    private void tearDown() {
        try {
            sdb.dropCollectionSpace( MAIN_CS_NAME );
            sdb.dropCollectionSpace( SUB_CS_NAME );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( cata != null ) {
                cata.close();
            }
        }
    }

    private void readyCL( CollectionSpace mCS, CollectionSpace sCS ) {
        // create mainCL
        BSONObject mOpt = new BasicBSONObject();
        mOpt.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        mOpt.put( "IsMainCL", true );
        mainCL = mCS.createCollection( CL_NAME_BASE + "mcl", mOpt );

        // create subCL
        BSONObject sOpt = new BasicBSONObject();
        sOpt.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        sOpt.put( "ShardingType", "hash" );
        sOpt.put( "Partition", 1024 );
        sOpt.put( "Group", CommLib.getDataGroupNames( sdb ).get( 0 ) );
        DBCollection sCL = sCS.createCollection( CL_NAME_BASE + "scl", sOpt );

        // attach subCL
        BSONObject attOpt = new BasicBSONObject();
        attOpt.put( "LowBound", new BasicBSONObject( "a", 0 ) );
        attOpt.put( "UpBound", new BasicBSONObject( "a", 100 ) );
        mainCL.attachCollection( sCL.getFullName(), attOpt );

        // create nomal cl
        for ( int i = 0; i < CL_NUM; i++ ) {
            sCS.createCollection( CL_NAME_BASE + i, sOpt );
        }
    }
}
