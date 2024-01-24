package com.sequoiadb.fulltext.parallel;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
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
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-12122:部分集合上存在全文索引，多个集合同时执行增删改/truncate/lob操作
 * @Author
 * @Date liuxiaoxuan 2019.5.10
 */
public class Fulltext12122 extends FullTestBase {
    private List< DBCollection > cls = new ArrayList<>();
    private String textIndexName = "fulltext12122";
    private List< String > cappedNames = new ArrayList<>();
    private List< String > esIndexNames = new ArrayList<>();
    ThreadExecutor te = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
    }

    @Override
    protected void caseInit() throws Exception {
        // 创建集合空间和集合，总共两个集合空间，每个集合空间对应2个集合
        for ( int csNum = 0; csNum < 2; csNum++ ) {
            String csName = "cs12122_" + csNum;
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName );
            for ( int clNum = 0; clNum < 2; clNum++ ) {
                String clName = "12122_cl_" + clNum;
                DBCollection cl = cs.createCollection( clName );
                FullTextDBUtils.insertData( cl, 10000 );
                if ( clNum % 2 > 0 ) {
                    BSONObject indexObj = new BasicBSONObject();
                    indexObj.put( "a", "text" );
                    cl.createIndex( textIndexName, indexObj, false, false );
                }
                cls.add( cl );
            }
        }
    }

    @Override
    protected void caseFini() throws Exception {
        for ( int csNum = 0; csNum < 2; csNum++ ) {
            String csName = "cs12122_" + csNum;
            FullTextDBUtils.dropCollectionSpace( sdb, csName );
        }
        for ( int i = 0; i < esIndexNames.size(); i++ ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb,
                    esIndexNames.get( i ), cappedNames.get( i ) ) );
        }
    }

    @Test
    public void test() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexCreated( cls.get( 1 ),
                textIndexName, 10000 ) );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cls.get( 3 ),
                textIndexName, 10000 ) );

        cappedNames.add(
                FullTextDBUtils.getCappedName( cls.get( 1 ), textIndexName ) );
        cappedNames.add(
                FullTextDBUtils.getCappedName( cls.get( 3 ), textIndexName ) );
        esIndexNames.add(
                FullTextDBUtils.getESIndexName( cls.get( 1 ), textIndexName ) );
        esIndexNames.add(
                FullTextDBUtils.getESIndexName( cls.get( 3 ), textIndexName ) );

        List< TruncateThread > truncateThreads = new ArrayList<>();
        for ( int csNum = 0; csNum < 2; csNum++ ) {
            String csName = "cs12122_" + csNum;
            for ( int clNum = 0; clNum < 2; clNum++ ) {
                String clName = "12122_cl_" + clNum;
                if ( clNum % 2 > 0 ) {
                    // truncate集合，且集合存在全文索引
                    TruncateThread truncateThread = new TruncateThread( csName,
                            clName );
                    truncateThreads.add( truncateThread );
                }
                te.addWorker( new LobThread( csName, clName ) );
                te.addWorker( new InsertThread( csName, clName ) );
                te.addWorker( new UpdateThread( csName, clName ) );
                te.addWorker( new DeleteThread( csName, clName ) );
            }
        }

        te.run();

        // 插入数据正常
        FullTextDBUtils.insertData( cls.get( 1 ), 1000 );
        FullTextDBUtils.insertData( cls.get( 3 ), 1000 );

        // 检查最终ES端全文索引是否完成同步、原始集合和固定集合主备数据是否一致
        Assert.assertTrue( FullTextUtils.isIndexCreated( cls.get( 1 ),
                textIndexName, ( int ) cls.get( 1 ).getCount() ) );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cls.get( 3 ),
                textIndexName, ( int ) cls.get( 3 ).getCount() ) );

        // 全文检索结果正确
        BSONObject matcher = ( BSONObject ) JSON
                .parse( "{'':{'$Text':{'query':{'match_all':{}}}}}" );
        DBCursor cursor = cls.get( 1 ).query( matcher, null, null, null );
        int count = 0;
        while ( cursor.hasNext() ) {
            cursor.getNext();
            count++;
        }
        Assert.assertEquals( count, ( int ) cls.get( 1 ).getCount() );
    }

    class TruncateThread extends ResultStore {
        private String csName;
        private String clName;

        public TruncateThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "清空原始集合")
        public void truncate() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncate();
            } catch ( BaseException e ) {
                if ( -321 != e.getErrorCode() && -190 != e.getErrorCode() ) {
                    Assert.fail( "actual exception: " + e.getErrorCode() );
                }
                saveResult( e.getErrorCode(), e );
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class LobThread {
        private String csName;
        private String clName;

        public LobThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "插入lob")
        public void createLob() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            Sequoiadb sdb = null;
            DBLob lob = null;
            try {
                sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                String lobStringBuff = StringUtils
                        .getRandomString( new Random().nextInt( 1024 ) );
                lob = cl.createLob();
                lob.write( lobStringBuff.getBytes() );
            } catch ( BaseException e ) {
                if ( -321 != e.getErrorCode() && -190 != e.getErrorCode() ) {
                    Assert.fail( "actual exception: " + e.getErrorCode() );
                }
            } finally {
                if ( lob != null ) {
                    lob.close();
                }
                if ( sdb != null ) {
                    sdb.close();
                }
                System.out.println(
                        this.getClass().getName().toString() + " end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class InsertThread {
        String csName = null;
        String clName = null;

        public InsertThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "往原始集合插入数据")
        public void insert() {
            System.out.println(
                    this.getClass().getName().toString() + " insert begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                List< BSONObject > insertObjs = new ArrayList< BSONObject >();
                int insertRecordNum = 10000;
                String strA = StringUtils.getRandomString( 64 );
                for ( int i = 0; i < insertRecordNum; i++ ) {
                    insertObjs.add( ( BSONObject ) JSON.parse( "{ a: '" + strA
                            + "', b: 'new_insert_12122_" + i + "'}" ) );
                }
                cl.insert( insertObjs, 0 );
            } catch ( BaseException e ) {
                if ( -321 != e.getErrorCode() ) {
                    Assert.fail( "actual exception: " + e.getErrorCode() );
                }
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " insert end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class UpdateThread {
        String csName = null;
        String clName = null;

        public UpdateThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "更新记录")
        public void update() {
            System.out.println(
                    this.getClass().getName().toString() + " update begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject modifier = new BasicBSONObject( "$set",
                        new BasicBSONObject( "a",
                                "fulltext12122_after_update" ) );
                BSONObject matcher = new BasicBSONObject( "id",
                        new BasicBSONObject( "$lt", 2000 ) );
                cl.update( matcher, modifier, null );
            } catch ( BaseException e ) {
                if ( -321 != e.getErrorCode() ) {
                    Assert.fail( "actual exception: " + e.getErrorCode() );
                }
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " update end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class DeleteThread {
        String csName = null;
        String clName = null;

        public DeleteThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "删除记录")
        public void delete() {
            System.out.println(
                    this.getClass().getName().toString() + " delete begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject matcher = new BasicBSONObject( "id",
                        new BasicBSONObject( "$gt", 5000 ) );
                cl.delete( matcher );
            } catch ( BaseException e ) {
                if ( -321 != e.getErrorCode() ) {
                    Assert.fail( "actual exception: " + e.getErrorCode() );
                }
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " delete end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

}
