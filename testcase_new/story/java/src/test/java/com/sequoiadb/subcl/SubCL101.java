package com.sequoiadb.subcl;

import java.util.ArrayList;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-101:使用JAVA驱动批量插入时，根据_id进行数据切分(hash)
 * @author huangxiaoni
 * @date 2019.3.19
 * @review
 *
 */

public class SubCL101 extends SdbTestBase {
    private Sequoiadb sdb;
    private DBCollection mCL;
    private String srcRg;
    private String dstRg;
    private final static String DOMAIN_NAME = "dm101";
    private final static String CS_NAME = "cs101";
    private final static String MAINCL_NAME = "mcl";
    private final static String SUBCL_NAME = "scl";
    private final static int RECORDS_NUM = 10000;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "The mode is standlone, "
                    + "or only one group, skip the testCase." );
        }

        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcRg = groupNames.get( 0 );
        dstRg = groupNames.get( 1 );

        // create domain
        String[] clRgNames = { srcRg, dstRg };
        BSONObject dmOpt = new BasicBSONObject();
        dmOpt.put( "Groups", clRgNames );
        dmOpt.put( "AutoSplit", true );
        sdb.createDomain( DOMAIN_NAME, dmOpt );

        // create cs
        BSONObject csOpt = new BasicBSONObject();
        csOpt.put( "Domain", DOMAIN_NAME );
        CollectionSpace cs = sdb.createCollectionSpace( CS_NAME, csOpt );

        // create mainCL/subCL/attachCL
        mCL = cs.createCollection( MAINCL_NAME, ( BSONObject ) JSON
                .parse( "{IsMainCL:true, ShardingKey:{_id:1}}" ) );
        DBCollection sCL = cs.createCollection( SUBCL_NAME, ( BSONObject ) JSON
                .parse( "{ShardingKey:{_id:1}, ShardingType:'hash'}" ) );
        mCL.attachCollection( sCL.getFullName(), ( BSONObject ) JSON.parse(
                "{LowBound:{_id:{$minKey:1}}, UpBound:{_id:{$maxKey:1}}}" ) );
    }

    @Test
    private void test() {
        this.insertData();
        // check results
        Sequoiadb srcRgDB = null;
        Sequoiadb trgRgDB = null;
        long srcRecCnt = 0;
        long trgRecCnt = 0;
        try {
            srcRgDB = sdb.getReplicaGroup( srcRg ).getMaster().connect();
            srcRecCnt = srcRgDB.getCollectionSpace( CS_NAME )
                    .getCollection( SUBCL_NAME ).getCount();

            trgRgDB = sdb.getReplicaGroup( dstRg ).getMaster().connect();
            trgRecCnt = trgRgDB.getCollectionSpace( CS_NAME )
                    .getCollection( SUBCL_NAME ).getCount();
        } finally {
            if ( srcRgDB != null )
                srcRgDB.close();
            if ( trgRgDB != null )
                trgRgDB.close();
        }

        Assert.assertEquals( srcRecCnt + trgRecCnt, RECORDS_NUM );

        long diffVal = Math.abs( srcRecCnt - trgRecCnt );
        long maxCnt = Math.max( srcRecCnt, trgRecCnt );
        int diffPerc = ( int ) ( ( ( float ) diffVal / maxCnt ) * 100 );
        Assert.assertTrue( diffPerc < 30,
                "percentage difference, expect 30, but actual " + diffPerc );
    }

    @AfterClass
    private void tearDown() {
        try {
            sdb.dropCollectionSpace( CS_NAME );
            sdb.dropDomain( DOMAIN_NAME );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void insertData() {
        ArrayList< BSONObject > insertor = new ArrayList< BSONObject >();
        for ( int i = 0; i < RECORDS_NUM; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "a", i );
            insertor.add( record );
        }
        mCL.insert( insertor );
    }
}
