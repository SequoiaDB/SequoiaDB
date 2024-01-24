package com.sequoiadb.fulltext.largedata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: Fulltext11995.java test content: 反复重建删除同一个全文索引
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.20
 */
public class Fulltext11995 extends FullTestBase {

    private String clName = "ES_11995";

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Test
    public void test() throws Exception {
        FullTextDBUtils.insertData( cl, FullTextUtils.INSERT_NUMS );

        String textIndexName = "fulltext11995";
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );

        String esIndexName = null;
        String cappedName = null;

        // 先插入数据后创建全文索引会引起全量索引，此时会同步原始集合的数据
        // 在未同步数据、同步原始集合数据的过程中，反复创建删除全文索引
        int doTimes = 10;
        while ( --doTimes > 0 ) {
            cl.createIndex( textIndexName, indexObj, false, false );
            esIndexName = FullTextDBUtils.getESIndexName( cl, textIndexName );
            cappedName = FullTextDBUtils.getCappedName( cl, textIndexName );
            FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
        }

        // 在创建全文索后，再插入数据会引起增量索引，此时会同步固定集合的数据
        // 同步固定集合数据的过程中，反复创建删除全文索引
        doTimes = 5;
        int newInsertNums = 100000;
        while ( --doTimes > 0 ) {
            cl.createIndex( textIndexName, indexObj, false, false );

            InsertThread insertThread = new InsertThread( newInsertNums );
            DropIndexThread dropIdxThread = new DropIndexThread();
            insertThread.start();
            dropIdxThread.start();

            Assert.assertTrue( insertThread.isSuccess(),
                    insertThread.getErrorMsg() );
            Assert.assertTrue( dropIdxThread.isSuccess(),
                    dropIdxThread.getErrorMsg() );
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
        }

        // 最后一次创建索引
        cl.createIndex( textIndexName, indexObj, false, false );
        // 检查ES数据是否完成同步，主备节点上原始集合、固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                ( int ) cl.getCount() ) );

        // 最后一次删除索引
        FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
        // 检查索引在ES端已被清除，固定集合不存在，主备节点上原始集合的数据一致
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON
                        .parse( "{a: 'test_11995_" + i * j + "', b: '"
                                + StringUtils.getRandomString( 32 ) + "', c: '"
                                + StringUtils.getRandomString( 64 ) + "', d: '"
                                + StringUtils.getRandomString( 64 ) + "', e: '"
                                + StringUtils.getRandomString( 128 ) + "', f: '"
                                + StringUtils.getRandomString( 128 ) + "'}" ) );
            }
            cl.insert( insertObjs, 0 );
            insertObjs.clear();
        }
    }

    private class InsertThread extends SdbThreadBase {

        int insertNums = 0;

        public InsertThread( int insertNums ) {
            this.insertNums = insertNums;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBCollection cl = null;
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            insertData( cl, insertNums );
            db.close();
        }
    }

    private class DropIndexThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                String textIndexName = "fulltext11995";
                // 删除全文索引
                FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
            }
        }

    }
}
