package com.sequoiadb.fulltext.parallel;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-12127:创建/删除集合与创建/删除全文索引并发
 * @Author
 * @Date liuxiaoxuan 2019.5.10
 */
public class Fulltext12127 extends FullTestBase {
    private List< DBCollection > cls = new ArrayList<>();
    private String textIndexName = "fulltext12127";
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
            String csName = "cs12127_" + csNum;
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName );
            for ( int clNum = 0; clNum < 2; clNum++ ) {
                String clName = "12127_cl_" + clNum;
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
            String csName = "cs12127_" + csNum;
            FullTextDBUtils.dropCollectionSpace( sdb, csName );
        }
        for ( int i = 0; i < esIndexNames.size(); i++ ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb,
                    esIndexNames.get( i ), cappedNames.get( i ) ) );
        }
    }

    @Test
    public void test() throws Exception {
        List< DropTextIndexThread > dropTextIndexThreads = new ArrayList<>();
        List< DropCLThread > dropCLThreads = new ArrayList<>();

        for ( int csNum = 0; csNum < 2; csNum++ ) {
            String csName = "cs12127_" + csNum;
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            // 在每个集合空间下创建新的集合
            te.addWorker( new CreateCLThread( csName, "12127_new_cl" ) );
            for ( int clNum = 0; clNum < 2; clNum++ ) {
                String clName = "12127_cl_" + clNum;
                DBCollection cl = cs.getCollection( clName );
                // 删除原有集合
                DropCLThread dropCLThread = new DropCLThread( csName, clName );
                if ( clNum % 2 > 0 ) {
                    // 在已存在全文索引的集合中，获取固定集合名和全文索引名
                    cappedNames.add( FullTextDBUtils.getCappedName( cl,
                            textIndexName ) );
                    esIndexNames.add( FullTextDBUtils.getESIndexName( cl,
                            textIndexName ) );
                    DropTextIndexThread dropTextIndexThread = new DropTextIndexThread(
                            csName, clName );
                    dropTextIndexThreads.add( dropTextIndexThread );
                    // 删除全文索引
                    te.addWorker( dropTextIndexThread );
                    // 删除集合，且集合中存在全文索引
                    te.addWorker( dropCLThread );
                } else {
                    // 不存在全文索引的集合上创建索引
                    te.addWorker( new CreateTextIndexThread( csName, clName ) );
                }
            }
        }

        te.run();

        // 获取被删除的有全文索引的CL
        List< DropCLThread > newDropCLThreads = new ArrayList<>();
        for ( int i = 0; i < dropCLThreads.size(); i++ ) {
            if ( i % 2 != 0 ) {
                newDropCLThreads.add( dropCLThreads.get( i ) );
            }
        }

        for ( int i = 0; i < newDropCLThreads.size(); i++ ) {
            // 集合依然存在的情况下，且全文索引删除成功，执行全文检索报错
            if ( newDropCLThreads.get( i ).getRetCode() != 0
                    && dropTextIndexThreads.get( i ).getRetCode() == 0 ) {
                Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb,
                        esIndexNames.get( i ), cappedNames.get( i ) ) );
                DBCollection cl = cls.get( i );
                // 插入数据正常
                FullTextDBUtils.insertData( cl, 100 );
                BSONObject matcher = ( BSONObject ) JSON
                        .parse( "{'':{'$Text':{'query':{'match_all':{}}}}}" );
                DBCursor cursor = cl.query( matcher, null, null, null );

                try {
                    cursor = cl.query( matcher, null, null, null );
                    Assert.fail( "query should fail" );
                } catch ( BaseException e ) {
                    if ( -6 != e.getErrorCode() && -52 != e.getErrorCode() 
                            && -10 != e.getErrorCode() ) {
                        throw e;
                    }
                } finally {
                    if ( cursor != null ) {
                        cursor.close();
                    }
                }
            }
            // 集合依然存在的情况下，且全文索引删除失败，全文检索结果正确
            else if ( newDropCLThreads.get( i ).getRetCode() != 0
                    && dropTextIndexThreads.get( i ).getRetCode() != 0 ) {
                int count = 0;
                DBCursor cursor = null;
                try {
                    DBCollection cl = cls.get( i * 2 + 1 );
                    // 插入数据正常
                    FullTextDBUtils.insertData( cl, 100 );
                    Assert.assertTrue( FullTextUtils.isIndexCreated( cl,
                            textIndexName, ( int ) cl.getCount() ) );
                    BSONObject matcher = ( BSONObject ) JSON.parse(
                            "{'':{'$Text':{'query':{'match_all':{}}}}}" );
                    cursor = cl.query( matcher, null, null, null );
                    while ( cursor.hasNext() ) {
                        cursor.getNext();
                        count++;
                    }
                    Assert.assertEquals( count, ( int ) cl.getCount() );
                } finally {
                    if ( cursor != null ) {
                        cursor.close();
                    }
                }
            }
            // 集合不存在
            else if ( newDropCLThreads.get( i ).getRetCode() != 0 ) {
                Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb,
                        esIndexNames.get( i ), cappedNames.get( i ) ) );
            }
        }
    }

    class DropTextIndexThread extends ResultStore {
        private String csName;
        private String clName;

        public DropTextIndexThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "删除全文索引")
        public void dropTextIndex() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( textIndexName );
            } catch ( BaseException e ) {
                if ( -147 != e.getErrorCode() && -23 != e.getErrorCode() ) {
                    throw e;
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

    class CreateTextIndexThread {
        private String csName;
        private String clName;

        public CreateTextIndexThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "创建全文索引")
        public void createTextIndex() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );

            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject indexObj = new BasicBSONObject();
                indexObj.put( "a", "text" );
                cl.createIndex( textIndexName, indexObj, false, false );
            } catch ( BaseException e ) {
                if ( -23 != e.getErrorCode() ) {
                    throw e;
                }
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class CreateCLThread {
        private String csName = null;
        private String clName = null;

        public CreateCLThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "创建集合")
        public void createCL() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                sdb.getCollectionSpace( csName ).createCollection( clName );
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class DropCLThread extends ResultStore {
        private String csName = null;
        private String clName = null;

        public DropCLThread( String csName, String clName ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "删除集合")
        public void dropCL() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                sdb.getCollectionSpace( csName ).dropCollection( clName );
            } catch ( BaseException e ) {
                if ( -147 != e.getErrorCode() ) {
                    throw e;
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
}
