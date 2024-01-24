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
 * FileName: Fulltext14376.java test content: 正在处理固定集合中记录，插入/修改/删除/查询集合中的记录
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.21
 */
public class Fulltext14376 extends FullTestBase {

    private String clName = "ES_14376";
    private String cappedName = null;
    private String esIndexName = null;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
    }

    @Override
    protected void caseFini() throws Exception {
        // 检查全文索引是否残留
        if ( esIndexName != null ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
        }
    }

    @Test
    public void test() throws Exception {
        // 先创建索引，再插入数据，触发增量索引
        String textIndexName = "fulltext14376";
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        indexObj.put( "g", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );

        cappedName = FullTextDBUtils.getCappedName( cl, textIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, textIndexName );

        insertData( cl, FullTextUtils.INSERT_NUMS );

        // 当ES在同步固定集合的时候执行增删改操作
        InsertThread insertThread = new InsertThread();
        UpdateThread updateThread = new UpdateThread();
        RemoveThread removeThread = new RemoveThread();

        insertThread.start();
        updateThread.start();
        removeThread.start();

        Assert.assertTrue( insertThread.isSuccess(),
                insertThread.getErrorMsg() );
        Assert.assertTrue( updateThread.isSuccess(),
                updateThread.getErrorMsg() );
        Assert.assertTrue( removeThread.isSuccess(),
                removeThread.getErrorMsg() );

        // 检查ES端索引数据是否完成同步，主备节点上主表的原始集合、固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                ( int ) cl.getCount() ) );
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse( "{a: 'test_14376_"
                        + i * j + "', b: '" + StringUtils.getRandomString( 32 )
                        + "', c: '" + StringUtils.getRandomString( 64 )
                        + "', d: '" + StringUtils.getRandomString( 64 )
                        + "', e: '" + StringUtils.getRandomString( 128 )
                        + "', f: '" + StringUtils.getRandomString( 128 )
                        + "', g: " + i * j + "}" ) );
            }
            cl.insert( insertObjs, 0 );
            insertObjs.clear();
        }
    }

    private class InsertThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = null;
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                int insertNums = 100000;
                insertData( cl, insertNums );
            }
        }
    }

    private class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = null;
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                BSONObject modifier = new BasicBSONObject( "$set",
                        new BasicBSONObject( "g", "-1" ) );
                BSONObject matcher = new BasicBSONObject( "g",
                        new BasicBSONObject( "$lt", 100000 ) );
                cl.update( matcher, modifier, null );
            }
        }
    }

    private class RemoveThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = null;
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                BSONObject matcher = new BasicBSONObject( "g",
                        new BasicBSONObject( "$gt", 100000 ) );
                cl.delete( matcher );
            }
        }
    }

}
