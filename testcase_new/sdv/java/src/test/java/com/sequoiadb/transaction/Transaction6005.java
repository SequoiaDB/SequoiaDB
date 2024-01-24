package com.sequoiadb.transaction;

import java.util.ArrayList;

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
 * @description seqDB-6005:切分表执行事务操作，创建唯一索引插入相同记录
 * @author huangxiaoni
 * @date 2019.4.9
 * @review
 */

public class Transaction6005 extends SdbTestBase {
    private Sequoiadb sdb;
    private String srcRg;
    private String dstRg;
    private CollectionSpace cs;
    private DBCollection cl;
    private final static String CL_NAME = "trans_6005";
    private final static int DOCS_NUM = 1000;
    private final static String IDX_NAME = "idx";

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        ArrayList< String > groupsName = CommLib.getDataGroupNames( sdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }

        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcRg = groupNames.get( 0 );
        dstRg = groupNames.get( 1 );

        cs = sdb.getCollectionSpace( SdbTestBase.csName );

        BSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "range" );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "Group", srcRg );
        cl = cs.createCollection( CL_NAME, options );

        cl.insert( new BasicBSONObject( "a", -1 ) );
        cl.insert( new BasicBSONObject( "a", DOCS_NUM ) );

        cl.split( srcRg, dstRg, 50 );
    }

    @Test()
    private void test() throws Exception {
        sdb.beginTransaction();

        ArrayList< BSONObject > insertor = new ArrayList< BSONObject >();
        for ( int i = 0; i < DOCS_NUM; i++ ) {
            insertor.add( new BasicBSONObject( "a", i ) );
        }
        cl.insert( insertor );
        cl.delete( new BasicBSONObject( "a", -1 ) );
        cl.delete( new BasicBSONObject( "a", DOCS_NUM ) );

        cl.createIndex( IDX_NAME, new BasicBSONObject( "a", 1 ), true, true );
        try {
            cl.insert( insertor );
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            if ( -38 != e.getErrorCode() ) {
                throw e;
            }
        }

        sdb.commit();

        Assert.assertEquals( 2, cl.getCount() );
        Assert.assertTrue( cl.isIndexExist( IDX_NAME ) );
        // check srcRG records
        Sequoiadb srcDB = sdb.getReplicaGroup( srcRg ).getMaster().connect();
        DBCursor cr = srcDB.getCollectionSpace( SdbTestBase.csName )
                .getCollection( CL_NAME )
                .query( new BasicBSONObject( "a", -1 ), null, null, null );
        Assert.assertEquals( -1, cr.getNext().get( "a" ) );
        // check dstRG records
        Sequoiadb dstDB = sdb.getReplicaGroup( srcRg ).getMaster().connect();
        cr = dstDB.getCollectionSpace( SdbTestBase.csName )
                .getCollection( CL_NAME )
                .query( new BasicBSONObject( "a", -1 ), null, null, null );
        Assert.assertEquals( -1, cr.getNext().get( "a" ) );
    }

    @AfterClass
    private void tearDown() {
        try {
            cs.dropCollection( CL_NAME );
        } finally {
            sdb.commit();
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
