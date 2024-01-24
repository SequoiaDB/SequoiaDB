package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-22105:remove/update和dropIdIndex并发
 * @Author XiaoNi Huang 2020/4/23
 */

public class IdIndex22105 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_idIndex22105";
    private int recsNum = 10000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName,
                new BasicBSONObject( "ShardingType", "range" ).append(
                        "ShardingKey", new BasicBSONObject( "a", 1 ) ) );
        ArrayList< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < recsNum; i++ ) {
            insertor.add( new BasicBSONObject( "a", i ) );
        }
        cl.insert( insertor );
    }

    @SuppressWarnings("deprecation")
    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        int threadNum = 20;
        int threadRecsNum = recsNum / threadNum;
        int startRecsNum = 0;
        for ( int i = 0; i < threadNum; i++ ) {
            if ( i % 2 == 0 ) {
                es.addWorker( new RemoveRecs( startRecsNum, threadRecsNum ) );
            } else {
                es.addWorker( new UpdateRecs( startRecsNum, threadRecsNum ) );
            }

            startRecsNum += threadRecsNum;

        }
        es.addWorker( new DropIdIndex() );
        es.run();

        // check results
        Assert.assertFalse( cl.getIndex( "$id" ).hasNext() );

        cl.createIdIndex( new BasicBSONObject() );
        Assert.assertTrue( cl.getIndex( "$id" ).hasNext() );

        cl.delete( new BasicBSONObject() );
        Assert.assertEquals( cl.getCount(), 0 );

        runSuccess = true;
    }

    private class RemoveRecs {
        private int startRecsNum;
        private int recsNum;

        private RemoveRecs( int startRecsNum, int recsNum ) {
            this.startRecsNum = startRecsNum;
            this.recsNum = recsNum;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject matcher = ( BSONObject ) JSON.parse(
                        "{$and:[{a:{$gte:" + startRecsNum + "}},{a:{$lt:"
                                + ( startRecsNum + recsNum ) + "}}]}" );
                cl.delete( matcher );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RTN_AUTOINDEXID_IS_FALSE
                        .getErrorCode() ) {
                    throw e;
                }
            } finally {
                if ( db != null )
                    db.close();
            }
        }
    }

    private class UpdateRecs {
        private int startRecsNum;
        private int recsNum;

        private UpdateRecs( int startRecsNum, int recsNum ) {
            this.startRecsNum = startRecsNum;
            this.recsNum = recsNum;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject matcher = ( BSONObject ) JSON.parse(
                        "{$and:[{a:{$gte:" + startRecsNum + "}},{a:{$lt:"
                                + ( startRecsNum + recsNum ) + "}}]}" );
                BSONObject modifier = ( BSONObject ) JSON
                        .parse( "{$inc:{a:" + recsNum + "}, $set:{b:'test'}}" );
                cl.update( matcher, modifier, null );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RTN_AUTOINDEXID_IS_FALSE
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class DropIdIndex {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.dropIdIndex();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( !runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }
}