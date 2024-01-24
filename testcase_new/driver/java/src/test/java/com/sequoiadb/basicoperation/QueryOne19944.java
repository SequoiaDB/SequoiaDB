package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * @Descreption seqDB-19944:queryOne查询检查cursor
 * @Author huangxiaoni
 * @Date 2019.9.29
 */

public class QueryOne19944 extends SdbTestBase {
    private boolean runSuccess = false;
    private Random random = new Random();
    private Sequoiadb sdb;
    private String mcsName = "mcs19944";
    private String mclName = "mcl";
    private DBCollection mcl;
    private String scsName = "scs19944";
    private String sclNameBase = "scl";
    private int sclNum = 15;
    private int recordsNum = sclNum * 100;
    private int runTimes = 1500;
    private int expTmpContextNum = 1000;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Is standalone." );
        }

        // clear cs
        String[] csNames = { mcsName, scsName };
        for ( String csName : csNames ) {
            try {
                sdb.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }

        // create main cs/cl and sub cs/cl
        this.readyCSCL();
    }

    @Test
    private void test() {
        while ( runTimes-- > 0 ) {
            BSONObject cond = new BasicBSONObject( "a",
                    random.nextInt( recordsNum ) );
            BSONObject obj = mcl.queryOne( cond, null, null, null, 1024 );
            obj.toString();
        }

        DBCursor cursor = sdb.getList( 0,
                new BasicBSONObject( "Global", false ), null, null );
        while ( cursor.hasNext() ) {
            BasicBSONList contexts = ( BasicBSONList ) cursor.getNext()
                    .get( "Contexts" );
            if ( contexts.size() > expTmpContextNum ) {
                Assert.fail( "context may not close, current contexts size = "
                        + contexts.size() + ", contexts = " + contexts + "." );
            }
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( mcsName );
                sdb.dropCollectionSpace( scsName );
            }
        } finally {
            sdb.disconnect();
        }
    }

    private void readyCSCL() {
        // create main cs/cl
        CollectionSpace mcs = sdb.createCollectionSpace( mcsName );
        BasicBSONObject mclOpt = new BasicBSONObject();
        mclOpt.put( "ShardingType", "range" );
        mclOpt.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        mclOpt.put( "IsMainCL", true );
        mcl = mcs.createCollection( mclName, mclOpt );

        // create sub cs/cl, and attach sub cl
        CollectionSpace scs = sdb.createCollectionSpace( scsName );
        BasicBSONObject sclOpt = new BasicBSONObject();
        sclOpt.put( "ShardingType", "range" );
        sclOpt.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        for ( int i = 0; i < sclNum; i++ ) {
            String sclName = sclNameBase + i;
            scs.createCollection( sclName, sclOpt );

            BasicBSONObject attOpt = new BasicBSONObject();
            attOpt.put( "LowBound",
                    new BasicBSONObject( "a", ( recordsNum / sclNum ) * i ) );
            attOpt.put( "UpBound", new BasicBSONObject( "a",
                    ( recordsNum / sclNum ) * ( i + 1 ) ) );
            mcl.attachCollection( scsName + "." + sclName, attOpt );
        }

        // attach sub cl
        for ( int i = 0; i < sclNum; i++ ) {
        }

        // insert records
        List< BSONObject > insertor = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordsNum; i++ ) {
            insertor.add( new BasicBSONObject( "a", i ) );
        }
        mcl.bulkInsert( insertor, 0 );
    }
}
