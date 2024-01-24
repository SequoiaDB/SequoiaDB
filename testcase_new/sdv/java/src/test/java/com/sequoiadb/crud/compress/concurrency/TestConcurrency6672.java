package com.sequoiadb.crud.compress.concurrency;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @descreption seqDB-6672:并发做增删改查操作_st.compress.04.001
 * @author linsuqiang
 * @date 2016-12-22
 * @updateUser YiPan
 * @updateDate 2021/12/10
 * @updateRemark 关闭未使用的游标，优化抛错流程
 * @version 1.0
 */
public class TestConcurrency6672 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6672";
    private String ranStr = CompressUtils.getRandomString( 8 * 1024 );
    private String dataGroupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CompressUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DBCollection cl = createCL();
        insertData( cl, 9000 );
        CompressUtils.waitCreateDict( cl, dataGroupName );
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    /**
     * before test, 9000 records were ready and it's range is [0,9000) range [0
     * ,3000 ) is to delete range [3000,6000 ) is to update range [6000,9000 )
     * is to query range [9000,12000) will be added by doing insert
     */
    @Test
    public void test() {
        DeleteThread deleteThread = new DeleteThread();
        UpdateThread updateThread = new UpdateThread();
        QueryThread queryThread = new QueryThread();
        InsertThread insertThread = new InsertThread();

        deleteThread.start();
        updateThread.start();
        queryThread.start();
        insertThread.start();

        if ( !( deleteThread.isSuccess() && updateThread.isSuccess()
                && queryThread.isSuccess() && insertThread.isSuccess() ) ) {
            Assert.fail( deleteThread.getErrorMsg() + updateThread.getErrorMsg()
                    + queryThread.getErrorMsg() + insertThread.getErrorMsg() );
        }

        checkDeleted();
        checkUpdated();
        checkQueried();
        checkInserted();
    }

    private class DeleteThread extends SdbThreadBase {
        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                for ( int i = 0; i < 3000; i++ ) {
                    // do delete
                    BSONObject delMatcher = new BasicBSONObject();
                    delMatcher.put( "a", i );
                    cl.delete( delMatcher );
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class UpdateThread extends SdbThreadBase {
        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                for ( int i = 0; i < 3000; i++ ) {
                    // do update
                    BSONObject updMatcher = new BasicBSONObject();
                    updMatcher.put( "a", 3000 + i );
                    BSONObject updModifier = new BasicBSONObject();
                    updModifier.put( "$set", JSON.parse( "{b:'hahahahaha'}" ) );
                    cl.update( updMatcher, updModifier, null );
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class QueryThread extends SdbThreadBase {
        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                for ( int i = 0; i < 3000; i++ ) {
                    // do query
                    BSONObject qryMatcher = new BasicBSONObject();
                    qryMatcher.put( "a", 6000 + i );
                    DBCursor cursor = cl.query( qryMatcher, null, null, null );
                    Assert.assertEquals( cursor.getNext().get( "b" ), ranStr );
                    cursor.close();
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class InsertThread extends SdbThreadBase {
        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                for ( int i = 0; i < 3000; i++ ) {
                    // do insert
                    BSONObject insertOpt = new BasicBSONObject();
                    insertOpt.put( "a", 9000 + i );
                    insertOpt.put( "b", "Merry Christmas" );
                    cl.insert( insertOpt );
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private DBCollection createCL() {
        DBCollection cl = null;
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        BSONObject option = new BasicBSONObject();
        dataGroupName = ( CompressUtils.getDataGroups( sdb ) ).get( 0 );
        option.put( "Group", dataGroupName );
        option.put( "Compressed", true );
        option.put( "CompressionType", "lzw" );
        option.put( "ReplSize", 0 );
        cl = cs.createCollection( clName, option );
        return cl;
    }

    private void insertData( DBCollection cl, int recSum ) {
        for ( int i = 0; i < recSum; i++ ) {
            BSONObject rec = new BasicBSONObject();
            rec.put( "a", i );
            rec.put( "b", ranStr );
            cl.insert( rec );
        }
    }

    private void checkDeleted() {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        BSONObject delMatcher = new BasicBSONObject();
        delMatcher.put( "a", JSON.parse( "{$lt : 3000}" ) );
        if ( ( int ) cl.getCount( delMatcher ) != 0 ) {
            Assert.fail( "fail to delete" );
        }
    }

    private void checkUpdated() {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        // check count
        BSONObject updMatcher = new BasicBSONObject();
        updMatcher = ( BSONObject ) JSON
                .parse( "{$and:[{a:{$gte:3000}},{a:{$lt : 6000}}]}" );
        if ( ( int ) cl.getCount( updMatcher ) != 3000 ) {
            Assert.fail( "fail to delete" );
        }
        // check content
        BSONObject updSelector = ( BSONObject ) JSON
                .parse( "{_id:{$include:0}}" );
        BSONObject updOrder = ( BSONObject ) JSON.parse( "{a:1}" );
        DBCursor updCursor = cl.query( updMatcher, updSelector, updOrder,
                null );
        for ( int i = 3000; i < 6000; i++ ) {
            BSONObject expRec = new BasicBSONObject();
            expRec.put( "a", i );
            expRec.put( "b", "hahahahaha" );
            BSONObject actRec = updCursor.getNext();
            Assert.assertEquals( actRec, expRec, "updated data is different" );
        }
        updCursor.close();
    }

    private void checkQueried() {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        // check count
        BSONObject qryMatcher = new BasicBSONObject();
        qryMatcher = ( BSONObject ) JSON
                .parse( "{$and:[{a:{$gte:6000}},{a:{$lt : 9000}}]}" );
        if ( ( int ) cl.getCount( qryMatcher ) != 3000 ) {
            Assert.fail( "queried result is wrong" );
        }
        // check content
        BSONObject qrySelector = ( BSONObject ) JSON
                .parse( "{_id:{$include:0}}" );
        BSONObject qryOrder = ( BSONObject ) JSON.parse( "{a:1}" );
        DBCursor qryCursor = cl.query( qryMatcher, qrySelector, qryOrder,
                null );
        for ( int i = 6000; i < 9000; i++ ) {
            BSONObject expRec = new BasicBSONObject();
            expRec.put( "a", i );
            expRec.put( "b", ranStr );
            BSONObject actRec = qryCursor.getNext();
            Assert.assertEquals( actRec, expRec, "queried data is different" );
        }
        qryCursor.close();
    }

    private void checkInserted() {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        // check count
        BSONObject insMatcher = new BasicBSONObject();
        insMatcher = ( BSONObject ) JSON
                .parse( "{$and:[{a:{$gte:9000}},{a:{$lt : 12000}}]}" );
        if ( ( int ) cl.getCount( insMatcher ) != 3000 ) {
            Assert.fail( "fail to insert" );
        }
        // check content
        BSONObject insSelector = ( BSONObject ) JSON
                .parse( "{_id:{$include:0}}" );
        BSONObject insOrder = ( BSONObject ) JSON.parse( "{a:1}" );
        DBCursor insCursor = cl.query( insMatcher, insSelector, insOrder,
                null );
        for ( int i = 9000; i < 12000; i++ ) {
            BSONObject expRec = new BasicBSONObject();
            expRec.put( "a", i );
            expRec.put( "b", "Merry Christmas" );
            BSONObject actRec = insCursor.getNext();
            Assert.assertEquals( actRec, expRec, "inserted data is different" );
        }
        insCursor.close();
    }
}