package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 用例要求： 1、向cl中插入大量数据（如1千万条记录）
 * 2、创建排序方式ID索引，执行创建索引命令createIdIndex({SortBufferSize:32}) 3、创建索引过程中向该CL插入数据
 * 4、查看创建索引结果和插入数据情况
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class IdIndex6613 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "c6613";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        createCL();
    }

    @Test
    public void insertDataAndCreateIdIndex() {
        int beginNo1 = 0;
        int endNo1 = 10000;
        int beginNo2 = 10000;
        int endNo2 = 20000;
        int beginNo3 = 20000;
        int endNo3 = 30000;
        InsertDatas insertDatas1 = new InsertDatas( beginNo1, endNo1 );
        InsertDatas insertDatas2 = new InsertDatas( beginNo2, endNo2 );
        InsertDatas insertDatas3 = new InsertDatas( beginNo3, endNo3 );
        insertDatas1.start();
        insertDatas2.start();
        insertDatas3.start();

        CreateIndex createIndex = new CreateIndex();
        createIndex.start();

        Assert.assertTrue( insertDatas1.isSuccess(),
                insertDatas1.getErrorMsg() );
        Assert.assertTrue( insertDatas2.isSuccess(),
                insertDatas2.getErrorMsg() );
        Assert.assertTrue( insertDatas3.isSuccess(),
                insertDatas3.getErrorMsg() );
        Assert.assertTrue( createIndex.isSuccess(), createIndex.getErrorMsg() );

        // 检查插入要确保索引已建
        checkIndex( cl );
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    public void createCL() {
        try {
            String clOptions = "{ShardingKey:{a:1},ShardingType:'hash',Partition:1024,"
                    + "ReplSize:0,Compressed:true,AutoIndexId:false}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( this.clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    /**
     * 创建索引
     */
    private class CreateIndex extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            @SuppressWarnings("resource")
            Sequoiadb sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                DBCollection cl1 = sdb1.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject indexObj2 = ( BSONObject ) JSON
                        .parse( "{SortBufferSize:32}" );

                // 随机取2000-6000之间的sleep时间，验证插入不同阶段createindex
                try {
                    long time = ( long ) ( 2000
                            + Math.random() * ( 6000 - 2000 + 1 ) );
                    Thread.sleep( time );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
                cl1.createIdIndex( indexObj2 );
            } finally {
                if ( sdb1 != null ) {
                    sdb1.disconnect();
                }
            }
        }
    }

    private class InsertDatas extends SdbThreadBase {
        private int beginNo, endNo;

        private InsertDatas( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            @SuppressWarnings("resource")
            Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl2 = sdb2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            try {
                insertData( cl2, beginNo, endNo );
                // check the insert records,check the numbers
                String match = "{a:{$gte:" + beginNo + ",$lt:" + endNo + "}}";
                long count = cl2.getCount( match );
                Assert.assertEquals( count, endNo - beginNo );
            } finally {
                if ( sdb2 != null ) {
                    sdb2.disconnect();
                }
            }
        }
    }

    private void insertData( DBCollection cl, int beginNo, int endNo ) {
        List< BSONObject > list = new ArrayList<>();
        for ( int i = beginNo; i < endNo; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            obj.put( "b", i );
            obj.put( "c", i );
            obj.put( "test", "testeaaaaasdgadgaasdga" + i );
            obj.put( "str", "test_" + String.valueOf( i ) );
            // insert the decimal type data
            String str = "32345.067891234567890123456789" + i;
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimal", decimal );
            // the data type
            Date now = new Date();
            obj.put( "date", now );
            list.add( obj );
        }
        cl.insert( list );
    }

    /**
     * 检查索引
     */
    @SuppressWarnings("deprecation")
    public void checkIndex( DBCollection cl ) {
        DBCursor cursor1 = null;
        DBCursor cursorIndex = null;
        try {
            cursorIndex = cl.getIndex( "$id" );
            while ( cursorIndex.hasNext() ) {
                // check the $id index info
                BSONObject object = cursorIndex.getNext();
                BSONObject record = ( BSONObject ) object.get( "IndexDef" );
                boolean actualUnique = ( boolean ) record.get( "unique" );
                Assert.assertEquals( actualUnique, true );

                boolean actualEnforced = ( boolean ) record.get( "enforced" );
                Assert.assertEquals( actualEnforced, true );
                Assert.assertEquals( record.get( "key" ),
                        JSON.parse( "{'_id':1}" ) );
                // check the index status
                String indexFlag = ( String ) object.get( "IndexFlag" );
                Assert.assertEquals( indexFlag, "Normal" );
            }

            // 通过explain，判断是否走索引
            cursor1 = cl.explain( null, null, null,
                    ( BSONObject ) JSON.parse( "{'':'$id'}" ), 0, -1,
                    DBQuery.FLG_QUERY_FORCE_HINT, null );
            String scanType = null;
            String indexName = null;
            while ( cursor1.hasNext() ) {
                BSONObject record = cursor1.getNext();
                if ( record.get( "Name" )
                        .equals( SdbTestBase.csName + "." + clName ) ) {
                    scanType = ( String ) record.get( "ScanType" );
                    indexName = ( String ) record.get( "IndexName" );
                }
            }
            Assert.assertEquals( scanType, "ixscan" );
            Assert.assertEquals( indexName, "$id" );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
            if ( cursorIndex != null ) {
                cursorIndex.close();
            }
        }
    }
}
