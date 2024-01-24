package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 用例要求： 1、向cl中插入大量数据（如1千万条记录） 2、创建ID索引，执行创建索引命令createIdIndex()，默认不指定排序缓存参数
 * 3、创建索引过程中进行数据切分操作，索引键和切分键相同 4、查看创建索引结果和数据切分情况
 *
 * @author huangwenhua
 * @version 1.00
 * @Date 2016.12.14
 */
public class IdIndex6612 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "c6612";
    public String srcGroupName;
    private String destGroupName;
    private List< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        // 源组和目标组
        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        srcGroupName = groupsName.get( 0 );
        destGroupName = groupsName.get( 1 );
        createCL();
        insertData();
    }

    // 创建索引
    @SuppressWarnings({ "resource", "deprecation" })
    @Test
    public void createIndex() throws InterruptedException {
        Split splitThread = new Split();
        splitThread.start();
        Thread.sleep( 300 + new Random().nextInt( 200 ) );
        Sequoiadb db1 = null;
        try {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl1.createIdIndex( new BasicBSONObject() );
            checkIndex( cl1 );
        } finally {
            if ( db1 != null ) {
                db1.disconnect();
            }
        }

        Assert.assertTrue( splitThread.isSuccess(), splitThread.getErrorMsg() );
    }

    /**
     * 切分数据
     */
    class Split extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            Sequoiadb db2 = null;
            try {
                db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl2 = db2.getCollectionSpace( csName )
                        .getCollection( clName );
                cl2.split( srcGroupName, destGroupName, 50 );
                checkSplit();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -279 ) {
                    e.printStackTrace();
                    throw e;
                }

            } finally {
                if ( db2 != null ) {
                    db2.disconnect();
                }
            }
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {

        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    public void createCL() {
        String clOptions = "{ShardingKey:{\"_id\":1},ShardingType:'range',Partition:1024,"
                + "ReplSize:0,Compressed:true,AutoIndexId:false,Group:'"
                + srcGroupName + "'}";
        BSONObject options = ( BSONObject ) JSON.parse( clOptions );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( this.clName, options );
    }

    public void insertData() {
        List< BSONObject > bsonObjects = new ArrayList<>( 10000 );
        for ( int i = 0; i < 10000; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "_id", i );
            bson.put( "age", i );
            bson.put( "name", "Json" + i );
            bsonObjects.add( bson );
        }
        this.cl.insert( bsonObjects );
        this.insertRecods = bsonObjects;
    }

    /**
     * 检查索引
     */
    public void checkIndex( DBCollection cl ) {
        DBCursor cursor1 = null;
        // 通过explain，判断是否走索引
        cursor1 = cl.explain( null, null, null,
                ( BSONObject ) JSON.parse( "{'':'$id'}" ), 0, -1, 0, null );
        String scanType = null;
        String IndexName = null;
        while ( cursor1.hasNext() ) {
            BSONObject record = cursor1.getNext();
            if ( record.get( "Name" )
                    .equals( SdbTestBase.csName + "." + clName ) ) {
                scanType = ( String ) record.get( "ScanType" );
                IndexName = ( String ) record.get( "IndexName" );
            }
        }
        cursor1.close();
        Assert.assertEquals( scanType, "ixscan" );
        Assert.assertEquals( IndexName, "$id" );
    }

    /**
     * 检查切分
     */
    public void checkSplit() {
        Sequoiadb destDataNode = null;
        Sequoiadb srcDataNode = null;
        DBCursor cursor = null;
        // 目标组数据量检查
        destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                .connect();
        DBCollection destCL = destDataNode.getCollectionSpace( csName )
                .getCollection( clName );
        long count = destCL
                .getCount( "{$and:[{_id:{$gte:5000}},{_id:{$lt:10000}}]}" );
        Assert.assertEquals( count, 0 );// 切分失败，目标组数据应当不包含上述查询数据
        // 源组数据量检查
        srcDataNode = sdb.getReplicaGroup( srcGroupName ).getMaster().connect();
        DBCollection destCL2 = srcDataNode.getCollectionSpace( csName )
                .getCollection( clName );
        long count2 = destCL2
                .getCount( "{$and:[{_id:{$gte:0}},{_id:{$lt:10000}}]}" );
        Assert.assertEquals( count2, 10000 );// 切分失败，源组数据应当包含上述查询数据
        // coord数据查询
        cursor = this.cl.query( null, null, "{_id:1}", null );
        List< BSONObject > actual = new ArrayList<>();
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            actual.add( obj );
        }
        cursor.close();
        Assert.assertEquals( actual, this.insertRecods );
    }
}
