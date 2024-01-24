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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * FileName: Fulltext11990:主子表中创建/删除全文索引
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.27
 */
public class Fulltext11990 extends FullTestBase {
    private CollectionSpace cs = null;
    private DBCollection maincl = null;
    private String mainCLName = "ES_11990_maincl";
    private String subCLName1 = "ES_11990_subcl_1";
    private String subCLName2 = "ES_11990_subcl_2";
    private String subCLName3 = "ES_11990_subcl_3";
    private String srcGroupName = "";
    private String destGroupName = "";

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );

        // 创建主子表
        caseProp.setProperty( CLNAME, mainCLName );
        caseProp.setProperty( CLOPT,
                "{ShardingKey:{a:1}, ShardingType:'range', IsMainCL:true}" );
    }

    @Override
    protected void caseInit() throws Exception {
        maincl = sdb.getCollectionSpace( csName ).getCollection( mainCLName );
        ArrayList< String > groupsName = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupsName.get( 0 );
        destGroupName = groupsName.get( 1 );

        cs = sdb.getCollectionSpace( csName );
        cs.createCollection( subCLName1,
                ( BSONObject ) JSON.parse( "{Group:'" + srcGroupName + "'}" ) );
        DBCollection subcl = cs.createCollection( subCLName2,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{a0:1}, ShardingType:'hash', Group:'"
                                + srcGroupName + "'}" ) );
        subcl.split( srcGroupName, destGroupName, 50 );
        cs.createCollection( subCLName3, ( BSONObject ) JSON
                .parse( "{Group:'" + destGroupName + "'}" ) );
    }

    @Test
    public void test() throws Exception {
        // 挂载子表
        BSONObject options1 = ( BSONObject ) JSON
                .parse( "{LowBound:{a:'testa'}, UpBound:{a:'testa 999999'}}" );
        BSONObject options2 = ( BSONObject ) JSON
                .parse( "{LowBound:{a:'xxxa'}, UpBound:{a:'xxxa 999999'}}" );
        BSONObject options3 = ( BSONObject ) JSON
                .parse( "{LowBound:{a:'zzza'}, UpBound:{a:'zzza 999999'}}" );
        maincl.attachCollection( csName + "." + subCLName1, options1 );
        maincl.attachCollection( csName + "." + subCLName2, options2 );
        maincl.attachCollection( csName + "." + subCLName3, options3 );

        // 插入数据，数据分布在每个子表的各个数据组上
        insertData( maincl, FullTextUtils.INSERT_NUMS );

        // 在主表分区键上创建全文索引
        String textIndexName = "fulltext11990";
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        maincl.createIndex( textIndexName, indexObj, false, false );

        // 获取每个子表上的全文索引
        List< String > subCLFullNames = FullTextDBUtils.getSubCLNames( sdb,
                csName + "." + mainCLName );
        List< String > esIndexNames = new ArrayList<>();
        List< String > cappedCLNames = new ArrayList<>();
        for ( String subCLFullName : subCLFullNames ) {
            String subCSName = subCLFullName.split( "\\." )[ 0 ];
            String subCLName = subCLFullName.split( "\\." )[ 1 ];
            DBCollection subCL = sdb.getCollectionSpace( subCSName )
                    .getCollection( subCLName );
            esIndexNames.addAll(
                    FullTextDBUtils.getESIndexNames( subCL, textIndexName ) );
            String cappedCLName = FullTextDBUtils.getCappedName( subCL,
                    textIndexName );
            cappedCLNames.add( cappedCLName );
        }

        Assert.assertTrue( FullTextUtils.isMainCLIndexCreated( maincl,
                textIndexName, FullTextUtils.INSERT_NUMS ) );

        FullTextDBUtils.dropFullTextIndex( maincl, textIndexName );
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedCLNames ) );

        // 在子表的分区键和非分区键上创建全文索引
        indexObj = new BasicBSONObject();
        indexObj.put( "a0", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        maincl.createIndex( textIndexName, indexObj, false, false );

        Assert.assertTrue( FullTextUtils.isMainCLIndexCreated( maincl,
                textIndexName, FullTextUtils.INSERT_NUMS ) );

        FullTextDBUtils.dropFullTextIndex( maincl, textIndexName );
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedCLNames ) );
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 2 / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse( "{a: 'testa " + i * j
                        + "', a0:" + "'test_11990 " + i * j + "', b: '"
                        + StringUtils.getRandomString( 32 ) + "', c: '"
                        + StringUtils.getRandomString( 64 ) + "', d: '"
                        + StringUtils.getRandomString( 64 ) + "', e: '"
                        + StringUtils.getRandomString( 128 ) + "', f: '"
                        + StringUtils.getRandomString( 128 ) + "'}" ) );
            }
            for ( int j = 0; j < insertNums / 4 / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse( "{a: 'xxxa " + i * j
                        + "', a0:" + "'test_11990 " + i * j + "', b: '"
                        + StringUtils.getRandomString( 32 ) + "', c: '"
                        + StringUtils.getRandomString( 64 ) + "', d: '"
                        + StringUtils.getRandomString( 64 ) + "', e: '"
                        + StringUtils.getRandomString( 128 ) + "', f: '"
                        + StringUtils.getRandomString( 128 ) + "'}" ) );
            }
            for ( int j = 0; j < insertNums / 4 / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON.parse( "{a: 'zzza " + i * j
                        + "', a0:" + "'test_11990 " + i * j + "', b: '"
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
}
