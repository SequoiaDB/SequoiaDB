package com.sequoiadb.fulltext.largedata;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * FileName: Fulltext11981.java test content: 在非空集合中创建/删除全文索引
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.20
 */
public class Fulltext11981 extends FullTestBase {

    private String clName = "ES_11981";
    private final String textIndexName = "fulltext11981";
    private String cappedName = null;
    private String esIndexName = null;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Test
    public void test() throws Exception {
        insertDataLT32M();
        insertDataGT32MLT149M();
        insertDataGT149M();
    }

    /**
     * 插入32M以内的数据
     * 
     * @return void
     * @throws Exception
     */
    private void insertDataLT32M() throws Exception {
        int insertNums = 100000;
        FullTextDBUtils.insertData( cl, insertNums );

        // 创建索引
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );

        cappedName = FullTextDBUtils.getCappedName( cl, textIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, textIndexName );

        // 检查ES数据是否同步，且主备节点的原始集合和固定集合数据是否一致
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, textIndexName, insertNums ) );

        // 删除索引并检查索引是否被删除
        FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    /**
     * 插入大于32M小于149M的数据
     * 
     * @return void
     * @throws Exception
     */
    private void insertDataGT32MLT149M() throws Exception {
        int insertNums = 100000;
        FullTextDBUtils.insertData( cl, insertNums );

        // 创建索引
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );

        // 检查ES数据是否同步，且主备节点的原始集合和固定集合数据是否一致
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, textIndexName, 200000 ) );

        // 删除索引并检查索引是否被删除
        FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    /**
     * 插入大于149M的数据
     * 
     * @return void
     * @throws Exception
     */
    private void insertDataGT149M() throws Exception {
        int insertNums3 = 300000;
        FullTextDBUtils.insertData( cl, insertNums3 );

        // 创建索引
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );

        // 检查ES数据是否同步，且主备节点的原始集合和固定集合数据是否一致
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, textIndexName, 500000 ) );

        // 删除索引并检查索引是否被删除
        FullTextDBUtils.dropFullTextIndex( cl, textIndexName );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }
}
