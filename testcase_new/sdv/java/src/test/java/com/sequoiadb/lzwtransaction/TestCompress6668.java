package com.sequoiadb.lzwtransaction;

import java.util.ArrayList;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 1、CL压缩类型为lzw，开启事务，对CL做增删改查数据 2、执行事务回滚 3、检查返回结果，并检查数据压缩情况
 * 
 * @author chensiqin
 * @Date 2016-12-16
 */
public class TestCompress6668 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl6668";

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // 跳过 standAlone 和数据组不足的环境
            if ( CommLib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            ArrayList< String > groupsName = CommLib.getDataGroupNames( sdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 1、CL压缩类型为lzw，开启事务，对CL做增删改查数据 2、执行事务回滚 3、检查返回结果，并检查数据压缩情况
     */
    @Test
    public void test() {
        try {
            createCL();
            // 插入数据，使cl存在已被压缩的记录
            // 1048576
            LzwTransUtils util = new LzwTransUtils();
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

            util.insertData( this.cl, 109, 115, 1024 );
            // 做事务操作之前的压缩信息
            BSONObject before = getSnapshotDetail();
            Assert.assertEquals( before.get( "DictionaryCreated" ).toString(),
                    "true" );

            if ( ( double ) before.get( "CurrentCompressionRatio" ) >= 1 ) {
                Assert.fail( "CurrentCompressionRatio >= 1 !" );
            }
            // 开启事务
            this.sdb.beginTransaction();
            // 对cl做增删改查操作
            util.insertData( this.cl, 115, 116, 1024 );
            checkAdd();
            this.cl.update( "{_id:{$et:115}}", "{$inc:{age:2}}", "" );
            checkUpdate();
            this.cl.delete( "{_id:{$et:115}}" );
            checkDelete();
            // 执行回滚
            this.sdb.rollback();
            checkRollBack();
            // 回滚后记录跟开启事务前数据一致且记录压缩状态一致
            BSONObject after = getSnapshotDetail();
            Assert.assertEquals( before.get( "CompressionType" ).toString(),
                    after.get( "CompressionType" ).toString() );
            Assert.assertEquals( before.get( "DictionaryCreated" ).toString(),
                    after.get( "DictionaryCreated" ).toString() );
            if ( ( double ) after.get( "CurrentCompressionRatio" ) >= 1 ) {
                Assert.fail( "CurrentCompressionRatio >= 1 !" );
            }
            Assert.assertEquals( before.get( "TotalDataPages" ).toString(),
                    after.get( "TotalDataPages" ).toString() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.rollback();
        }
    }

    private void checkRollBack() {
        try {
            long count = this.cl.getCount();
            Assert.assertEquals( count, 115 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    private void checkDelete() {
        try {
            DBCursor cursor = this.cl.query(
                    ( BSONObject ) JSON.parse( "{_id:{$et:115}}" ), null, null,
                    null );
            Assert.assertEquals( cursor.hasNext(), false );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    private void checkUpdate() {
        try {
            DBCursor cursor = this.cl.query(
                    ( BSONObject ) JSON.parse( "{_id:{$et:115}}" ), null, null,
                    null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                Assert.assertEquals( obj.get( "age" ).toString(), "117" );
            }
            cursor.close();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void checkAdd() {
        try {
            DBCursor cursor = this.cl.query(
                    ( BSONObject ) JSON.parse( "{_id:{$et:114}}" ), null, null,
                    null );
            Assert.assertEquals( cursor.hasNext(), true );
            cursor.close();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

    }

    public BSONObject getSnapshotDetail() {
        BSONObject detail = null;
        Sequoiadb dataDB = null;
        try {
            detail = new BasicBSONObject();
            List< String > rgNames = LzwTransUtils.getDataRgNames( this.sdb );
            String url = LzwTransUtils.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 0 ) );
            dataDB = new Sequoiadb( url, "", "" );
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", csName + "." + clName );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            detail = ( BSONObject ) details.get( 0 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( dataDB != null ) {
                dataDB.close();
            }
        }
        return detail;
    }

    public void createCL() {
        try {
            List< String > rgNames = LzwTransUtils.getDataRgNames( this.sdb );
            BSONObject option = new BasicBSONObject();
            option.put( "Group", rgNames.get( 0 ) );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            this.cl = LzwTransUtils.createCL( this.cs, this.clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( this.clName ) ) {
                this.cs.dropCollection( this.clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.close();
        }
    }

}
