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
 * FileName: Fulltext14398.java test content: 集合删除后重建相同的全文索引
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.21
 */
public class Fulltext14398 extends FullTestBase {

    private CollectionSpace cs = null;
    private String clName = "ES_cl_14398";
    private String textIndexName = "fulltext14398";
    private String cappedName = null;
    private String esIndexName = null;
    private BSONObject indexObj = new BasicBSONObject();

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        cs = sdb.getCollectionSpace( csName );
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
        reCreateCLWhileESIndexNotExist();
        reCreateCLWhileESIndexExist();
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse(
                        "{a: 'test_14398_" + StringUtils.getRandomString( 10 )
                                + "', b: '" + StringUtils.getRandomString( 32 )
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

    private void reCreateCLWhileESIndexNotExist() throws Exception {
        // 创建集合并插入数据
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
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                FullTextUtils.INSERT_NUMS ) );

        // 删除集合并检查ES端索引已删除
        FullTextDBUtils.dropCollection( cs, clName );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );

        // 当ES索引不存在的时候，重建集合和全文索引
        cl = cs.createCollection( clName );
        cl.createIndex( textIndexName, indexObj, false, false );

        // 插入新数据
        int newInsertNums = 210000;
        insertData( cl, newInsertNums );

        // 检查ES端索引数据是否完成同步，主备节点上主表的原始集合、固定集合数据是否一致
        FullTextUtils.isIndexCreated( cl, textIndexName, newInsertNums );
    }

    private void reCreateCLWhileESIndexExist() throws Exception {
        // 初始化数据，先把上一次的全文索引删掉并把集合数据清零
        FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
        cl.truncate();
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );

        cl.createIndex( textIndexName, indexObj, false, false );
        insertData( cl, FullTextUtils.INSERT_NUMS );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                FullTextUtils.INSERT_NUMS ) );

        FullTextDBUtils.dropCollection( cs, clName );

        // 当ES索引还未删除的时候，重建集合和全文索引
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
