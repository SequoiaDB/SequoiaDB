package com.sequoiadb.fulltext.largedata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * FileName: Fulltext14397.java test content: 集合空间删除后重建相同的全文索引
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.21
 */
public class Fulltext14397 extends FullTestBase {

    private CollectionSpace cs = null;
    private String csName = "ES_cs_14397";
    private String clName = "ES_cl_14397";
    private String textIndexName = "fulltext14397";
    private String cappedName = null;
    private String esIndexName = null;
    private BSONObject indexObj = new BasicBSONObject();

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CSNAME, csName );
        caseProp.setProperty( CLNAME, clName );
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
        reCreateCSWhileESIndexNotExist();
        reCreateCSWhileESIndexExist();
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse( "{a: 'test_14397_"
                        + StringUtils.getRandomString( 10 ) + "', b: '"
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

    private void reCreateCSWhileESIndexNotExist() throws Exception {
        // 创建集合并插入数据
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );

        cappedName = FullTextDBUtils.getCappedName( cl, textIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, textIndexName );
        insertData( cl, FullTextUtils.INSERT_NUMS );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                FullTextUtils.INSERT_NUMS ) );

        // 删除集合空间并检查ES端索引已删除
        FullTextDBUtils.dropCollectionSpace( sdb, csName );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );

        // 当ES索引不存在的时候，重建集合空间和全文索引
        cs = sdb.createCollectionSpace( this.csName );
        cl = cs.createCollection( clName );
        cl.createIndex( textIndexName, indexObj, false, false );

        // 插入新数据
        int newInsertNums = 210000;
        insertData( cl, newInsertNums );

        // 检查ES端索引数据是否完成同步，主备节点上主表的原始集合、固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                newInsertNums ) );
    }

    private void reCreateCSWhileESIndexExist() throws Exception {
        // 初始化数据，先把上一次的全文索引删掉并把集合数据清零
        FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
        cl.truncate();
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );

        // 重新创建索引并插入数据
        cl.createIndex( textIndexName, indexObj, false, false );
        insertData( cl, FullTextUtils.INSERT_NUMS );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                FullTextUtils.INSERT_NUMS ) );

        FullTextDBUtils.dropCollectionSpace( sdb, csName );

        // 当ES索引还未删除的时候，重建集合空间和全文索引
        cs = sdb.createCollectionSpace( this.csName );
        cl = cs.createCollection( clName );
        cl.createIndex( textIndexName, indexObj, false, false );

        // 插入新数据
        int newInsertNums = 210000;
        insertData( cl, newInsertNums );

        // 检查ES端索引数据是否完成同步，主备节点上主表的原始集合、固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                newInsertNums ) );
    }

}
