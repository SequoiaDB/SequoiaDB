package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-15798:增删改查与truncate并发
 * @author yinzhen
 * @createDate 2019.04.22
 * @updateUser ZhangYanan
 * @updateDate 2021.12.14
 * @updateRemark
 * @version v1.0
 */
public class Fulltext15798 extends FullTestBase {
    private String clName = "cl15798";
    private String fullIdxName = "idx15798";
    private String esIndexName;
    private String cappedCLName;
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        insertData( cl, insertNum );
        cl.createIndex( fullIdxName,
                "{'a':'text','b':'text','c':'text','d':'text','e':'text','f':'text'}",
                false, false );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fullIdxName, insertNum ) );

        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
        cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thExecutor.addWorker( new TruncateCL() );
        thExecutor.addWorker( new InsertData() );
        thExecutor.addWorker( new UpdateData() );
        thExecutor.addWorker( new DeleteData() );
        thExecutor.addWorker( new QueryData() );

        thExecutor.run();

        // 主备节点数据一致
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIdxName,
                ( int ) cl.getCount() ) );
        // 全文检索校验
        FullTextUtils.isRecordEqualsByMulQueryMode( cl );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    private class InsertData {
        @ExecuteOrder(step = 1, desc = "增删改查记录包含全文索引字段")
        private void insertRecords() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, 10000 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class UpdateData {
        @ExecuteOrder(step = 1, desc = "增删改查记录包含全文索引字段")
        private void updateRecords() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.update( "{a:'test_15798_1'}", "{$set:{b:'b_15798'}}", "{}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DeleteData {
        @ExecuteOrder(step = 1, desc = "增删改查记录包含全文索引字段")
        private void deleteRecords() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.delete( "{a:'test_15798_0'}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class QueryData {
        @ExecuteOrder(step = 1, desc = "增删改查记录包含全文索引字段")
        private void queryRecords() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.query( "{'':{'$Text':{'query':{'match_all':{}}}}}", "{}",
                        "{}", "{'':'" + fullIdxName + "'}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -10
                        && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class TruncateCL {
        @ExecuteOrder(step = 1, desc = "增删改查记录的同时，truncate 集合中的记录")
        private void truncateCL() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                while ( cl.getCount() == 0 ) {
                    Thread.sleep( 100 );
                }

                int count = 0;
                while ( count++ < 600 ) {
                    try {
                        Thread.sleep( 100 );
                        cl.truncate();
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -190
                                && e.getErrorCode() != -147 ) {
                            throw e;
                        }
                        continue;
                    }
                    break;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                BSONObject record = ( BSONObject ) JSON
                        .parse( "{a: 'test_15798_" + i * j + "', b: '"
                                + StringUtils.getRandomString( 32 ) + "', c: '"
                                + StringUtils.getRandomString( 64 ) + "', d: '"
                                + StringUtils.getRandomString( 64 ) + "', e: '"
                                + StringUtils.getRandomString( 128 ) + "', f: '"
                                + StringUtils.getRandomString( 128 ) + "'}" );
                records.add( record );
            }
            cl.insert( records );
            records.clear();
        }
    }
}
