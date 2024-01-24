package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-15860:集合中存在全文索引，并发增删改/全文检索/查询/truncate记录/lob操作
 * @author luweikang
 * @createDate 2019.05.10
 * @updateUser ZhangYanan
 * @updateDate 2021.12.14
 * @updateRemark
 * @version v1.0
 */
public class Fulltext15860 extends FullTestBase {

    private String clName = "es_15860";
    private String indexName = "fulltextIndex15860";
    private int insertNum = 20000;
    private long lobSize = 1024 * 1024 * 10;
    private List< ObjectId > lobTruncateList = new ArrayList< ObjectId >();
    private List< ObjectId > lobRemoveList = new ArrayList< ObjectId >();
    private List< ObjectId > lobReadList = new ArrayList< ObjectId >();
    private List< ObjectId > lobPutList = new ArrayList< ObjectId >();
    private String esIndexName;
    private String cappedName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        List< ObjectId > lobList = writeLob( cl, 100 );
        lobTruncateList.addAll( lobList.subList( 0, 50 ) );
        lobRemoveList.addAll( lobList.subList( 50, 70 ) );
        lobReadList.addAll( lobList.subList( 70, 100 ) );
    }

    @Test
    public void test() throws Exception {

        FullTextDBUtils.insertData( cl, insertNum );

        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        cl.createIndex( indexName, indexObj, false, false );

        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, insertNum ) );

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new QueryByTextIndexThread() );
        thread.addWorker( new InsertThread() );
        thread.addWorker( new UpdateThread() );
        thread.addWorker( new DeleteThread() );
        thread.addWorker( new TruncateThread() );
        thread.addWorker( new TruncateLobThread() );
        thread.addWorker( new PutLobThread() );
        thread.addWorker( new RemoveLobThread() );
        thread.addWorker( new GetLoBThread() );
        thread.run();

        Assert.assertTrue( FullTextUtils.isCLConsistency( cl ) );
        Assert.assertTrue( FullTextUtils.isCLDataConsistency( cl ) );
        Assert.assertTrue( FullTextUtils.isRecordEqualsByMulQueryMode( cl ) );
        checkLobOpr();

        esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );
        cappedName = FullTextDBUtils.getCappedName( cl, indexName );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class InsertThread {

        @ExecuteOrder(step = 1)
        private void insert() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                List< BSONObject > insertObjs = new ArrayList< BSONObject >();
                String strB = StringUtils.getRandomString( 8 );
                String strC = StringUtils.getRandomString( 32 );
                String strD = StringUtils.getRandomString( 64 );
                String strE = StringUtils.getRandomString( 128 );
                int insertNum1 = insertNum + 1000;
                for ( int i = 0; i < 10; i++ ) {
                    for ( int j = insertNum; j < insertNum1; j++ ) {
                        int recordNum = i * 1000 + j;
                        insertObjs.add( ( BSONObject ) JSON
                                .parse( "{recordId: " + recordNum + ", a: '"
                                        + clName + recordNum + "', b: '" + strB
                                        + "', c: '" + strC + "', d: '" + strD
                                        + "', e: '" + strE + "'}" ) );
                    }
                    cl.insert( insertObjs, 0 );
                    insertObjs.clear();
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class UpdateThread {

        @ExecuteOrder(step = 1)
        private void update() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.update(
                        "{recordId: {$gte: 0, $lt: " + ( insertNum / 2 ) + "}}",
                        "{$set: {b: 'text'}}", null );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            }
        }
    }

    private class DeleteThread {

        @ExecuteOrder(step = 1)
        private void delete() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.delete( "{recordId: {$gte: " + ( insertNum / 2 ) + ", $lt: "
                        + insertNum + "}}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            }
        }
    }

    private class TruncateThread {

        @ExecuteOrder(step = 1)
        private void truncate() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncate();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class QueryByTextIndexThread {

        @ExecuteOrder(step = 1)
        private void queryData() throws InterruptedException {
            for ( int i = 0; i < 10; i++ ) {
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    Thread.sleep( 5500 + new Random().nextInt( 500 ) );
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    DBCursor cur = cl.query(
                            "{'': {'$Text': {'query': {'match_all': {}}}}}",
                            null, "{'recordId': 1}",
                            "{'': '" + indexName + "'}" );
                    while ( cur.hasNext() ) {
                        cur.getNext();
                    }
                    cur.close();
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -6 && e.getErrorCode() != -52
                            && e.getErrorCode() != -321
                            && e.getErrorCode() != -10
                            && e.getErrorCode() != -190 ) {
                        throw e;
                    }
                }
            }
        }
    }

    private class TruncateLobThread {

        @ExecuteOrder(step = 1)
        private void truncateLob() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Thread.sleep( 1000 + new Random().nextInt( 100 ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( ObjectId lobId : lobTruncateList ) {
                    cl.truncateLob( lobId, lobSize );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -4 && e.getErrorCode() != -321
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class PutLobThread {

        @ExecuteOrder(step = 1)
        private void putLob() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Thread.sleep( 1000 + new Random().nextInt( 100 ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                lobPutList.addAll( writeLob( cl, 100 ) );
            }
        }
    }

    private class RemoveLobThread {

        @ExecuteOrder(step = 1)
        private void removeLob() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Thread.sleep( 1000 + new Random().nextInt( 100 ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( ObjectId lobId : lobRemoveList ) {
                    cl.removeLob( lobId );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -4 && e.getErrorCode() != -321
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class GetLoBThread {

        @ExecuteOrder(step = 1)
        private void getLob() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Thread.sleep( 1000 + new Random().nextInt( 100 ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( ObjectId lobId : lobReadList ) {
                    DBLob lob = cl.openLob( lobId );
                    byte[] data = new byte[ ( int ) lobSize ];
                    lob.read( data );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -4 && e.getErrorCode() != -321
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    private List< ObjectId > writeLob( DBCollection cl, int lobNum ) {

        List< ObjectId > lobIdList = new ArrayList< ObjectId >();
        byte[] data = new byte[ ( int ) lobSize ];
        new Random().nextBytes( data );
        try {

            for ( int i = 0; i < lobNum; i++ ) {
                DBLob lob = cl.createLob();
                lob.write( data );
                lob.close();
                lobIdList.add( lob.getID() );
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -321 && e.getErrorCode() != -190 ) {
                throw e;
            }
        }

        return lobIdList;
    }

    private void checkLobOpr() {
        DBCursor lobCur = cl.listLobs();
        while ( lobCur.hasNext() ) {
            BSONObject lobInfo = lobCur.getNext();
            ObjectId id = ( ObjectId ) lobInfo.get( "Oid" );
            DBLob lob = cl.openLob( id );
            byte[] content = new byte[ ( int ) lobSize ];
            lob.read( content );
            lob.close();
        }
        lobCur.close();
    }

}
