package com.sequoiadb.lzw;

import java.util.List;

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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 1、创建cl，指定Compressed:true 2、切分过程中并发做增删改改查操作 3、操作完成后查看源组和目标组数据正确性
 *
 * @author chensiqin
 * @Date 2016-12-30
 */
public class TestSeqDB6971 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl6971";

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            LzwUilts1 util = new LzwUilts1();
            if ( util.isStandAlone( this.sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( LzwUilts1.getDataRgNames( this.sdb ).size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 1、切分过程中并发做增删改改查操作 2、操作完成后查看源组和目标组数据正确性
     */
    @Test
    public void test() {
        // 创建压缩cl
        createCL();
        this.cl.alterCollection( ( BSONObject ) JSON
                .parse( "{ShardingKey:{age:1},ShardingType:\"range\"}" ) );
        // 插入数据
        LzwUilts1 util = new LzwUilts1();
        util.insertData( this.cl, 0, 99, 1024 * 1024 );
        util.insertData( this.cl, 99, 109, 1024 * 1024 );
        BSONObject bObject = getSnapshotDetail();
        // wait for creating dictionary
        while ( !"true"
                .equals( bObject.get( "DictionaryCreated" ).toString() ) ) {
            try {
                Thread.sleep( 10 * 1000 );
                bObject = getSnapshotDetail();
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }

        SplitThread splitThread = new SplitThread();
        // 切分与增删改查并发
        AUQDThread auqdthread = new AUQDThread();
        splitThread.start();
        auqdthread.start();

        Assert.assertTrue( splitThread.isSuccess(), splitThread.getErrorMsg() );
        Assert.assertTrue( auqdthread.isSuccess(), auqdthread.getErrorMsg() );
        // 校验压缩状态
        checkCompress();
        // 校验源组结果
        checkSrcDataSplitResult();
        // 校验目标组结果
        checkDestDataSplitResult();
    }

    @SuppressWarnings({ "deprecation", "resource" })
    private void checkDestDataSplitResult() {
        Sequoiadb dataDb = null;
        try {
            // 连接源组data验证数据
            List< String > rgNames = LzwUilts1.getDataRgNames( this.sdb );
            String url = LzwUilts1.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 1 ) );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = cs.getCollection( this.clName );
            long ct = dbcl.getCount();
            long count = dbcl
                    .getCount( "{$and:[{age:{$gte:0}},{age:{$lt:100}}]}" );
            Assert.assertEquals( ct, 100 );
            Assert.assertEquals( count, 100 );
        } finally {
            dataDb.disconnect();
        }
    }

    @SuppressWarnings({ "resource", "deprecation" })
    private void checkSrcDataSplitResult() {
        Sequoiadb dataDb = null;
        try {
            // 连接源组data验证数据
            List< String > rgNames = LzwUilts1.getDataRgNames( this.sdb );
            String url = LzwUilts1.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 0 ) );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = cs.getCollection( this.clName );
            long ct = dbcl.getCount();
            long count = dbcl
                    .getCount( "{$and:[{age:{$gte:100}},{age:{$lt:114}}]}" );
            Assert.assertEquals( ct, 14 );
            Assert.assertEquals( count, 14 );
            // 验证更新字段
            DBCursor cursor = this.cl.query(
                    ( BSONObject ) JSON.parse( "{_id:{$et:112}}" ), null, null,
                    null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                Assert.assertEquals( obj.get( "num" ).toString(), "114" );
            }
            cursor.close();
        } finally {
            dataDb.disconnect();
        }
    }

    private void checkCompress() {
        BSONObject detail = getSnapshotDetail();
        Assert.assertEquals( detail.get( "CompressionType" ).toString(),
                "lzw" );
        Assert.assertEquals( detail.get( "DictionaryCreated" ).toString(),
                "true" );
        if ( ( double ) detail.get( "CurrentCompressionRatio" ) >= 1 ) {
            Assert.fail( "CurrentCompressionRatio >= 1 !" );
        }

    }

    public void createCL() {
        try {
            List< String > rgNames = LzwUilts1.getDataRgNames( this.sdb );
            BSONObject option = new BasicBSONObject();
            option.put( "Group", rgNames.get( 0 ) );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            this.cl = LzwUilts1.createCL( this.cs, this.clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @SuppressWarnings({ "resource", "deprecation" })
    public BSONObject getSnapshotDetail() {
        BSONObject detail = null;
        Sequoiadb dataDB = null;
        try {
            detail = new BasicBSONObject();
            List< String > rgNames = LzwUilts1.getDataRgNames( this.sdb );
            String url = LzwUilts1.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 0 ) );
            dataDB = new Sequoiadb( url, "", "" );
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", SdbTestBase.csName + "." + this.clName );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            detail = ( BSONObject ) details.get( 0 );
        } finally {
            if ( dataDB != null ) {
                dataDB.disconnect();
            }
        }
        return detail;
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( this.clName ) ) {
                this.cs.dropCollection( this.clName );
            }
        } finally {
            this.sdb.disconnect();
        }
    }

    class SplitThread extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() {
            Sequoiadb db2 = null;
            try {
                db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                List< String > rgNames2 = LzwUilts1.getDataRgNames( db2 );
                CollectionSpace cs2 = db2
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl2 = cs2.getCollection( clName );
                BSONObject startCondition = ( BSONObject ) JSON
                        .parse( "{age:0}" );
                BSONObject endCondition = ( BSONObject ) JSON
                        .parse( "{age:100}" );
                cl2.split( rgNames2.get( 0 ), rgNames2.get( 1 ), startCondition,
                        endCondition );
            } finally {
                db2.disconnect();
            }
        }
    }

    class AUQDThread extends SdbThreadBase {
        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace cs1 = db1
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl1 = cs1.getCollection( clName );
                LzwUilts1 util = new LzwUilts1();
                // 插入数据开始压缩
                util.insertData( cl1, 109, 115, 1024 );
                cl1.update( "{_id:{$et:112}}", "{$inc:{num:2}}", "" );
                cl1.delete( "{_id:{$et:114}}" );
            } finally {
                db1.disconnect();
            }
        }
    }
}
