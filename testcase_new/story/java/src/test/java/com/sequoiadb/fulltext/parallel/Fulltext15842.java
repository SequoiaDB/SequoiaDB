package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName FullText15842.java 创建全文索引与lob操作并发
 * @Author luweikang
 * @Date 2019年5月10日
 */
public class Fulltext15842 extends FullTestBase {

    private String clName = "es_15842";
    private String indexName = "fulltextIndex15842";
    private int insertNum = 100000;
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
        FullTextDBUtils.insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new CreateIndexThread() );
        thread.addWorker( new TruncateLobThread() );
        thread.addWorker( new PutLobThread() );
        thread.addWorker( new RemoveLobThread() );
        thread.addWorker( new GetLoBThread() );
        thread.run();

        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, insertNum ) );

        checkLobResult();

        int recordNum = 0;
        DBCursor cur = cl.query(
                "{'': {'$Text': {'query': {'match_all': {}}}}}", null,
                "{'recordId': 1}", "{'': '" + indexName + "'}" );
        while ( cur.hasNext() ) {
            cur.getNext();
            recordNum++;
        }
        cur.close();

        Assert.assertEquals( recordNum, insertNum,
                "use fulltext index search record" );
        cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class CreateIndexThread {

        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject indexObj = new BasicBSONObject();
                indexObj.put( "a", "text" );
                indexObj.put( "b", "text" );
                indexObj.put( "c", "text" );
                indexObj.put( "d", "text" );
                indexObj.put( "e", "text" );
                cl.createIndex( indexName, indexObj, false, false );
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
            }
        }
    }

    private List< ObjectId > writeLob( DBCollection cl, int lobNum ) {

        List< ObjectId > lobIdList = new ArrayList< ObjectId >();
        byte[] data = new byte[ ( int ) lobSize ];
        new Random().nextBytes( data );

        for ( int i = 0; i < lobNum; i++ ) {
            DBLob lob = cl.createLob();
            lob.write( data );
            lob.close();
            lobIdList.add( lob.getID() );
        }

        return lobIdList;
    }

    private void checkLobResult() {
        List< ObjectId > expLobIdList = new ArrayList< ObjectId >();
        expLobIdList.addAll( lobTruncateList );
        expLobIdList.addAll( lobReadList );
        expLobIdList.addAll( lobPutList );

        List< ObjectId > actLobIdList = new ArrayList< ObjectId >();
        DBCursor lobCur = cl.listLobs();
        while ( lobCur.hasNext() ) {
            actLobIdList.add( ( ObjectId ) lobCur.getNext().get( "Oid" ) );
        }

        sortLobIdList( expLobIdList );
        sortLobIdList( actLobIdList );

        Assert.assertEquals( actLobIdList.toString(), expLobIdList.toString() );
    }

    private void sortLobIdList( List< ObjectId > lobIdList ) {

        Collections.sort( lobIdList, new Comparator< ObjectId >() {
            @Override
            public int compare( ObjectId obj1, ObjectId obj2 ) {
                String str1 = obj1.toString();
                String str2 = obj2.toString();
                if ( str1.compareToIgnoreCase( str2 ) < 0 ) {
                    return -1;
                }
                return 1;
            }
        } );
    }

}
