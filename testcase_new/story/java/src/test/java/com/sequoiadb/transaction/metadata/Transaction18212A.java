package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18212:删除集合空间与事务操作并发
 * @date 2019-4-11
 * @author yinzhen
 *
 */
public class Transaction18212A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs18212A";
    private List< String > groupNames;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( 2 > groupNames.size() ) {
            throw new SkipException( "groups less than 2" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName );
        createCL( "cl18212A_A" );
        createCL( "cl18212B_A" );
        createCL( "cl18212C_A" );
    }

    @AfterClass
    public void tearDown() {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.close();
    }

    @Test
    public void test() {
        // 开启并发事务
        OperatorTh operatorTh1 = new OperatorTh( "cl18212A_A" );
        operatorTh1.start();

        OperatorTh operatorTh2 = new OperatorTh( "cl18212B_A" );
        operatorTh2.start();

        OperatorTh operatorTh3 = new OperatorTh( "cl18212C_A" );
        operatorTh3.start();

        DropCSTh dropCLTh = new DropCSTh();
        dropCLTh.start();

        Assert.assertTrue( operatorTh1.isSuccess(), operatorTh1.getErrorMsg() );
        Assert.assertTrue( operatorTh2.isSuccess(), operatorTh2.getErrorMsg() );
        Assert.assertTrue( operatorTh3.isSuccess(), operatorTh3.getErrorMsg() );
        Assert.assertTrue( dropCLTh.isSuccess(), dropCLTh.getErrorMsg() );
    }

    private void createCL( String clName ) {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName, ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit: true}" ) );
        cl.createIndex( "idx18212", "{a:1}", false, false );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" ) );
    }

    private class OperatorTh extends SdbThreadBase {
        private String clName;
        private Sequoiadb db;

        private OperatorTh( String clName ) {
            this.clName = clName;
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        @Override
        public void exec() throws Exception {
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                TransUtils.beginTransaction( db );
                insertDatas( cl, 10000, 20000 );
                cl.delete( "{$and:[{a:{$gte:0}},{a:{$lt:5000}}]}",
                        "{'':'idx18212'}" );
                cl.update( "{$and:[{a:{$gte:5000}},{a:{$lt:15000}}]}",
                        "{$inc:{a:10}}", "{}'':'idx18212'" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -190 && e.getErrorCode() != -23
                        && e.getErrorCode() != -34
                        && e.getErrorCode() != -248 ) {
                    throw e;
                }
            } finally {
                db.commit();
                db.close();
            }
        }
    }

    // 在事务内删除集合空间
    private class DropCSTh extends SdbThreadBase {
        private Sequoiadb db;

        private DropCSTh() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        @Override
        public void exec() throws Exception {
            try {
                while ( true ) {
                    try {
                        Thread.sleep( 1000 );
                        TransUtils.beginTransaction( db );
                        db.dropCollectionSpace( csName );
                        db.commit();
                        break;
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -190
                                && e.getErrorCode() != -147 ) {
                            throw e;
                        }
                    }
                }
            } finally {
                db.commit();
                db.close();
            }
        }
    }

    private void insertDatas( DBCollection cl, int startId, int endId ) {
        List< BSONObject > records = new ArrayList< >();
        for ( int i = startId; i < endId; i++ ) {
            records.add( ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" ) );
        }
        cl.insert( records );
    }
}
