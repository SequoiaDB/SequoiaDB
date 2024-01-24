package com.sequoiadb.fulltext.largedata;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-15799:主表执行truncate清空记录
 * @author yinzhen
 * @date 2018/11/28
 */
public class Fulltext15799 extends FullTestBase {
    private DBCollection mainCL;
    private String mainCLName = "maincl15799";
    private String fullIndexName = "fullIndex15799";
    private CollectionSpace cs = null;
    private String subCLName1 = "subcl15799A";
    private String subCLName2 = "subcl15799B";
    private List< String > esIndexNames01;
    private List< String > esIndexNames02;
    private String cappedCSName01;
    private String cappedCSName02;

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
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        mainCL = cs.getCollection( mainCLName );
    }

    @Test
    public void test() throws Exception {
        // 创建主子表，子表覆盖：普通表、切分表
        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        DBCollection subCL1 = cs.createCollection( subCLName1 );
        DBCollection subCL2 = cs.createCollection( subCLName2,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash',Group:'"
                                + groupNames.get( 0 ) + "'}" ) );
        subCL2.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
        mainCL.attachCollection( csName + "." + subCLName1, ( BSONObject ) JSON
                .parse( "{LowBound:{a:0}, UpBound:{a:114298}}" ) );
        mainCL.attachCollection( csName + "." + subCLName2, ( BSONObject ) JSON
                .parse( "{LowBound:{a:114298}, UpBound:{a:200001}}" ) );

        // 创建全文索引，索引字段覆盖：子表分区键、子表普通字段
        mainCL.createIndex( fullIndexName, "{\"b\":\"text\", \"c\":\"text\"}",
                false, false );
        esIndexNames01 = FullTextDBUtils.getESIndexNames( subCL1,
                fullIndexName );
        esIndexNames02 = FullTextDBUtils.getESIndexNames( subCL2,
                fullIndexName );
        cappedCSName01 = FullTextDBUtils.getCappedName( subCL1, fullIndexName );
        cappedCSName02 = FullTextDBUtils.getCappedName( subCL2, fullIndexName );

        // 插入包含全文索引字段的记录
        insertData( FullTextUtils.INSERT_NUMS );
        Assert.assertTrue( FullTextUtils.isMainCLIndexCreated( mainCL,
                fullIndexName, FullTextUtils.INSERT_NUMS ) );

        mainCL.truncate();
        Assert.assertTrue( FullTextUtils.isMainCLIndexCreated( mainCL,
                fullIndexName, 0 ) );
    }

    @Override
    protected void caseFini() throws Exception {
        esIndexNames01.addAll( esIndexNames02 );
        List< String > cappedNames = new ArrayList<>();
        cappedNames.add( cappedCSName01 );
        cappedNames.add( cappedCSName02 );
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames01,
                cappedNames ) );
    }

    private void insertData( int insertNums ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        Random random = new Random();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                BSONObject record = ( BSONObject ) JSON
                        .parse( "{a:" + random.nextInt( 200000 ) + ",b:'b" + i
                                + "" + j + "', c:'c" + i + "" + j + "'}" );
                records.add( record );
            }
            mainCL.insert( records );
            records.clear();
        }
    }
}