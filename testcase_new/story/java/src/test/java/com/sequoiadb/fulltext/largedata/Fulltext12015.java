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
 * FileName: Fulltext12015.java test content: 主子表中插入/更新/删除包含全文索引字段的记录
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.27
 */
public class Fulltext12015 extends FullTestBase {

    private CollectionSpace cs = null;
    private DBCollection maincl = null;
    private String mainCLName = "ES_12015_maincl";
    private String subCLName1 = "ES_12015_subcl_1";
    private String subCLName2 = "ES_12015_subcl_2";
    private String textIndexName = "fulltext12015";
    private List< String > cappedNames = new ArrayList<>();
    private List< String > esIndexNames = new ArrayList<>();

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );

        caseProp.setProperty( CLNAME, mainCLName );
        caseProp.setProperty( CLOPT,
                "{ShardingKey:{a:1}, ShardingType:'range', IsMainCL:true}" );
    }

    @Override
    protected void caseInit() throws Exception {
        // 创建主子表
        cs = sdb.getCollectionSpace( csName );
        maincl = cs.getCollection( mainCLName );
        cs.createCollection( subCLName1 );
        cs.createCollection( subCLName2, ( BSONObject ) JSON
                .parse( "{ShardingKey:{a0:1}, ShardingType:'hash'}" ) );

        // 挂载子表
        BSONObject options1 = ( BSONObject ) JSON
                .parse( "{LowBound:{a:'testa'}, UpBound:{a:'testa 999999'}}" );
        BSONObject options2 = ( BSONObject ) JSON
                .parse( "{LowBound:{a:'zzza'}, UpBound:{a:'zzza 999999'}}" );
        maincl.attachCollection( csName + "." + subCLName1, options1 );
        maincl.attachCollection( csName + "." + subCLName2, options2 );
    }

    @Override
    protected void caseFini() throws Exception {
        // 检查全文索引是否残留
        if ( esIndexNames != null ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                    cappedNames ) );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    @Test
    public void test() throws Exception {
        // 在主表的分区键和非分区键上创建全文索引
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "a0", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        maincl.createIndex( textIndexName, indexObj, false, false );

        // 获取每个子表的全文索引
        List< String > subCLFullNames = FullTextDBUtils.getSubCLNames( sdb,
                csName + "." + mainCLName );
        for ( String subCLFullName : subCLFullNames ) {
            String subCSName = subCLFullName.split( "\\." )[ 0 ];
            String subCLName = subCLFullName.split( "\\." )[ 1 ];
            DBCollection subCL = sdb.getCollectionSpace( subCSName )
                    .getCollection( subCLName );
            esIndexNames.addAll(
                    FullTextDBUtils.getESIndexNames( subCL, textIndexName ) );
            cappedNames.add(
                    FullTextDBUtils.getCappedName( subCL, textIndexName ) );
        }

        insertData( maincl, FullTextUtils.INSERT_NUMS );

        // 检查ES端索引数据是否完成同步，主备节点上主表的原始集合、固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isMainCLIndexCreated( maincl,
                textIndexName, FullTextUtils.INSERT_NUMS ) );

        // 更新数据，更新后再次插入
        update( maincl );
        insertData( maincl, 10000 );

        // 检查ES端索引数据是否完成同步，主备节点上主表的原始集合、固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isMainCLIndexCreated( maincl,
                textIndexName, FullTextUtils.INSERT_NUMS + 10000 ) );
        remove( maincl );

        // 检查ES端索引数据是否完成同步，主备节点上主表的原始集合、固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isMainCLIndexCreated( maincl,
                textIndexName, ( int ) maincl.getCount() ) );
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 2 / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse( "{a: 'testa " + i * j
                        + "', a0:" + "'test_12051 " + i * j + "', b: '"
                        + StringUtils.getRandomString( 32 ) + "', c: '"
                        + StringUtils.getRandomString( 64 ) + "', d: '"
                        + StringUtils.getRandomString( 64 ) + "', e: '"
                        + StringUtils.getRandomString( 128 ) + "', f: '"
                        + StringUtils.getRandomString( 128 ) + "'}" ) );
            }
            for ( int j = 0; j < insertNums / 2 / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse( "{a: 'zzza " + i * j
                        + "', a0:" + "'test_12051 " + i * j + "', b: '"
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

    private void update( DBCollection cl ) {
        BSONObject modifier = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        BSONObject matcher = new BasicBSONObject();
        BSONObject subMatcher = new BasicBSONObject();
        value.put( "a", "testa 99999" );
        modifier.put( "$set", value );
        subMatcher.put( "$lt", "testa 10000" );
        matcher.put( "a", subMatcher );
        cl.update( matcher, modifier, null );
    }

    private void remove( DBCollection cl ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject subMatcher = new BasicBSONObject();
        subMatcher.put( "$et", "testa 99999" );
        matcher.put( "a", subMatcher );
        cl.delete( matcher );
    }
}
