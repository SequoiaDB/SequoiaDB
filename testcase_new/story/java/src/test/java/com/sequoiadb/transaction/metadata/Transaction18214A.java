package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18214:lob操作与事务操作并发
 * @date 2019-4-11
 * @author yinzhen
 *
 */
public class Transaction18214A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18214A";
    private DBCollection cl = null;
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

        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit: true}" ) );
        cl.createIndex( "idx18214", "{a:1}", false, false );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" ) );
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启并发事务
        OperatorTh operatorTh = new OperatorTh();
        operatorTh.start();

        DropCLTh dropCLTh = new DropCLTh();
        dropCLTh.start();

        Assert.assertTrue( operatorTh.isSuccess(), operatorTh.getErrorMsg() );
        Assert.assertTrue( dropCLTh.isSuccess(), dropCLTh.getErrorMsg() );

    }

    private class OperatorTh extends SdbThreadBase {
        private Sequoiadb db;
        private DBCollection cl;

        private OperatorTh() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @Override
        public void exec() throws Exception {
            try {
                TransUtils.beginTransaction( db );
                insertDatas( cl, 10000, 20000 );
                cl.delete( "{$and:[{a:{$gte:0}},{a:{$lt:5000}}]}",
                        "{'':'idx18214'}" );
                cl.update( "{$and:[{a:{$gte:5000}},{a:{$lt:15000}}]}",
                        "{$inc:{a:10}}", "{}'':'idx18214'" );
            } finally {
                db.commit();
                db.close();
            }
        }
    }

    // 在事务内操作 LOB
    private class DropCLTh extends SdbThreadBase {
        private Sequoiadb db;

        private DropCLTh() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        @Override
        public void exec() throws Exception {
            Thread.sleep( 1000 );
            try {
                TransUtils.beginTransaction( db );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                DBLob lob = cl.createLob();
                lob.write( "test lob to test".getBytes() );
                lob.close();
                ObjectId oid = new ObjectId();
                DBLob lob2 = cl.createLob( oid );
                lob2.close();
                cl.removeLob( oid );
                cl.truncateLob( lob.getID(), 12 );
            } finally {
                db.commit();
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
