package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
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

/**
 * @Descreption seqDB-19951:queryOne指定flag为-1，查询多个分区组上的数据
 * @Author huangxiaoni
 * @Date 2019.10.10
 */

public class QueryOne19951 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb;
    private ArrayList< String > groupNames;
    private String csName = "cs19951";
    private String clName = "cl";
    private DBCollection cl;
    private int recordsNum = 1000;

    @BeforeClass
    private void setUp() {

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "standalone or only one group." );
        }
        groupNames = CommLib.getDataGroupNames( sdb );

        // create sharding cl
        try {
            sdb.dropCollectionSpace( csName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -34 ) {
                throw e;
            }
        }
        this.readyCL();

        // insert
        List< BSONObject > insertor = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordsNum; i++ ) {
            insertor.add( new BasicBSONObject( "a", i ) );
        }
        cl.bulkInsert( insertor, 0 );

        // split
        cl.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
    }

    @Test
    private void test() {
        Random random = new Random();
        int num;
        BSONObject obj;
        int flag;

        // flag: -1
        flag = -1;
        num = random.nextInt( recordsNum );
        obj = cl.queryOne( new BasicBSONObject( "a", num ), null, null, null,
                flag );
        Assert.assertEquals( obj.get( "a" ), num );

        // flag: 4096
        flag = 4096;
        num = random.nextInt( recordsNum );
        obj = cl.queryOne( new BasicBSONObject( "a", num ), null, null, null,
                flag );
        Assert.assertEquals( obj.get( "a" ), num );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            sdb.disconnect();
        }
    }

    private void readyCL() {
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "range" );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "Group", groupNames.get( 0 ) );
        cl = cs.createCollection( clName, options );
    }
}
