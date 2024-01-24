package com.sequoiadb.subcl;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestName:SEQDB-10552 子表在相同分区组上部分存在索引，通过主表进行索引操作
 *                       1.创建主子表，其中子表指定在相同的数据组上，且部分子表存在步骤2中的索引
 *                       2.通过主表创建索引，覆盖：_id索引，shard索引、普通索引，分别通过主表及子表查询索引信息
 *                       3.插入数据，分别指定步骤2中创建的索引进行查询，检查查询结果
 *                       4.通过主表删除索引，覆盖：_id索引，shard索引、普通索引，检查结果
 *                       5.通过步骤3中的查询条件，检查结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL10552 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL1;
    private DBCollection subCL2;
    private String mainCLName = "mainCL10552";
    private String subCL1Name = "subCL10552_1";
    private String subCL2Name = "subCL10552_2";
    private String oidPrefix = "515152ba49af3952000000";

    @BeforeClass(enabled = true)
    public void setUp() {
        try {
            sdb = new Sequoiadb( coordUrl, "", "" );
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException( "StandAlone skip this test:"
                        + this.getClass().getName() );
            }
            ArrayList< String > groupList = commlib.getDataGroupNames( sdb );
            commCS = sdb.getCollectionSpace( csName );

            // 创建主子表
            mainCL = commCS.createCollection( mainCLName, ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{sk:1}}" ) );
            subCL1 = commCS.createCollection( subCL1Name,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{sk:1},AutoIndexId:false,EnsureShardingIndex:false,Group:\""
                                    + groupList.get( 0 ) + "\"}" ) );
            subCL2 = commCS.createCollection( subCL2Name,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{sk:1},AutoIndexId:false,EnsureShardingIndex:false,Group:\""
                                    + groupList.get( 0 ) + "\"}" ) );
        } catch ( BaseException e ) {
            if ( sdb != null ) {
                sdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    @Test(enabled = true)
    public void test() {
        try {
            // 确认子表不存在索引
            checkIndexNonExist( subCL1 );
            checkIndexNonExist( subCL2 );

            // 为子表一创建索引，挂载所有子表
            createIndexAndAttach();

            // 通过主表创建索引，覆盖已有索引
            mainCL.createIndex( "shardingKeyIndex", "{sk:1}", false, false );// 覆盖shardingkey索引
            mainCL.createIdIndex( new BasicBSONObject() );// 覆盖id索引
            mainCL.createIndex( "commIndex", "{num:1}", false, false );// 覆盖普通字段索引

            // 分别通过主表，子表检查索引信息
            checkIndexExist( mainCL );
            checkIndexExist( subCL1 );
            checkIndexExist( subCL2 );

            // 插入数据，分别使用多种查询条件查询,比对结果,检查扫描方式
            insertData();
            // {sk:80}为查询条件，{sk:80,num:80,_id:{oid:"-"}}为期待的查询结果，ixscan为期待的扫描方式
            queryByIndexAndCheckExplain( "{sk:80}",
                    "{sk:80,num:80,_id:{$oid:\"" + oidPrefix + "80\"}}",
                    "ixscan" );
            queryByIndexAndCheckExplain( "{num:20}",
                    "{sk:20,num:20,_id:{$oid:\"" + oidPrefix + "20\"}}",
                    "ixscan" );
            queryByIndexAndCheckExplain( "{_id:{$oid:\"" + oidPrefix + "50\"}}",
                    "{sk:50,num:50,_id:{$oid:\"" + oidPrefix + "50\"}}",
                    "ixscan" );

            // 通过主表删除索引,检查删除结果
            mainCL.dropIdIndex();
            mainCL.dropIndex( "commIndex" );
            mainCL.dropIndex( "shardingKeyIndex" );
            checkIndexNonExist( mainCL );
            checkIndexNonExist( subCL1 );
            checkIndexNonExist( subCL2 );

            // 再次查询，检查扫描方式
            queryByIndexAndCheckExplain( "{sk:80}",
                    "{sk:80,num:80,_id:{$oid:\"" + oidPrefix + "80\"}}",
                    "tbscan" );// 通过id索引查询，期望扫描方式为ixscan
            queryByIndexAndCheckExplain( "{num:20}",
                    "{sk:20,num:20,_id:{$oid:\"" + oidPrefix + "20\"}}",
                    "tbscan" );
            queryByIndexAndCheckExplain( "{_id:{$oid:\"" + oidPrefix + "50\"}}",
                    "{sk:50,num:50,_id:{$oid:\"" + oidPrefix + "50\"}}",
                    "tbscan" );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + SubCLUtils2.getKeyStack( e, this )
                            + e.getStackTrace().toString() );
        }
    }

    private void insertData() {
        DecimalFormat df = new DecimalFormat( "00" );
        for ( int i = 0; i < 100; i++ ) {
            mainCL.insert( ( BSONObject ) JSON.parse( "{sk:" + i + ",num:" + i
                    + ",_id:{$oid:\"" + oidPrefix + df.format( i ) + "\"}}" ) );
        }
    }

    public void createIndexAndAttach() {
        try {
            subCL1.createIndex( "shardingKeyIndex", "{sk:1}", false, false );// 创建shardingkey索引
            subCL1.createIdIndex( new BasicBSONObject() );// 创建id索引
            subCL1.createIndex( "commIndex", "{num:1}", false, false );// 创建普通字段索引

            mainCL.attachCollection( subCL1.getFullName(), // 挂载
                    ( BSONObject ) JSON
                            .parse( "{LowBound:{sk:0},UpBound:{sk:50}}" ) );
            mainCL.attachCollection( subCL2.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk:50},UpBound:{sk:100}}" ) );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            commCS.dropCollection( subCL1Name );
            commCS.dropCollection( subCL2Name );
            commCS.dropCollection( mainCLName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    // 确认mainCL不存在索引
    private void checkIndexNonExist( DBCollection cl ) {
        DBCursor dbc = null;
        try {
            dbc = cl.getIndexes();
            if ( dbc.hasNext() ) {
                Assert.fail( "drop index fail" );
            }
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    // 按macher查询，结果与expectedRecord比对，检查访问计划的扫描方式是否为expectScanType
    private void queryByIndexAndCheckExplain( String macher,
            String expectedRecord, String expectScanType ) {
        DBCursor dbc1 = null;
        DBCursor dbc2 = null;
        BSONObject expected = ( BSONObject ) JSON.parse( expectedRecord );
        try {
            // 查询，检查结果的正确性
            dbc1 = mainCL.query( macher, null, null, null );
            ArrayList< BSONObject > queryReaults = new ArrayList< BSONObject >();
            while ( dbc1.hasNext() ) {
                queryReaults.add( dbc1.getNext() );
            }
            if ( queryReaults.size() == 1 ) {
                Assert.assertEquals( expected.equals( queryReaults.get( 0 ) ),
                        true, "expected:" + expected.toString() + " actual:"
                                + queryReaults.get( 0 ) );
            } else {
                Assert.fail(
                        "query resault not correct,the array:" + queryReaults );
            }

            // 检查扫描方式
            dbc2 = mainCL.explain( ( BSONObject ) JSON.parse( macher ), null,
                    null, null, 0, -1, 0, null );
            if ( dbc2.hasNext() ) {
                BasicBSONList list = ( BasicBSONList ) dbc2.getNext()
                        .get( "SubCollections" );
                BSONObject message = ( BSONObject ) list.get( 0 );
                Assert.assertEquals(
                        message.get( "ScanType" ).equals( expectScanType ),
                        true, "scanType not " + expectScanType );
            } else {
                Assert.fail( "mainCL explain wrong" );
            }
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc1 != null ) {
                dbc1.close();
            }
            if ( dbc2 != null ) {
                dbc2.close();
            }
        }
    }

    // 检查mainCL是否存在test方法中设置的索引
    public void checkIndexExist( DBCollection cl ) {
        DBCursor dbc = null;
        BSONObject idIndex = ( BSONObject ) JSON
                .parse( "{name: \"$id\",key: {_id: 1}}" );
        BSONObject shardingKeyIndex = ( BSONObject ) JSON
                .parse( "{name: \"shardingKeyIndex\",key: {sk: 1}}" );
        BSONObject commIndex = ( BSONObject ) JSON
                .parse( "{name: \"commIndex\",key: {num: 1}}" );
        ArrayList< BSONObject > expect = new ArrayList< BSONObject >();
        expect.add( idIndex );
        expect.add( shardingKeyIndex );
        expect.add( commIndex );
        boolean machFlag = false;
        try {
            dbc = cl.getIndexes();
            while ( dbc.hasNext() ) {
                BSONObject record = ( BSONObject ) dbc.getNext()
                        .get( "IndexDef" );
                BasicBSONObject actual = new BasicBSONObject();
                actual.put( "name", record.get( "name" ) );
                actual.put( "key", record.get( "key" ) );
                for ( int i = 0; i < expect.size(); i++ ) {
                    if ( actual.equals( expect.get( i ) ) ) {
                        machFlag = true;
                        expect.remove( i );
                        break;
                    }
                }
                if ( machFlag == false ) {
                    Assert.fail( "cant not find " + actual.get( "name" )
                            + "index message" );
                }
                machFlag = false;
            }
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }
}
