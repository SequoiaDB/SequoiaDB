package com.sequoiadb.lzw;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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

/**
 * 数据压缩支持decimal类型
 * 
 * @author chensiqin
 *
 */
public class TestSeqDB9331 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl9331";

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
     * 1、插入大量数据（>100万以达到构建字典条件），记录字段类型为decimal，覆盖如下场景： 1）decimal默认精度，如{ "key" :
     * { $decimal:"123.456" } } 2）decimal总位数为1000，小数位为999位
     * 3、查询数据，检查压缩后数据正确性（包括有压缩和未被压缩的记录）
     */
    @Test
    public void test() {
        try {
            createCL();
            // 插入数据，使cl存在已被压缩的记录
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

            util.insertData( this.cl, 109, 112, 1024 * 1024 );
            BSONObject detail = getSnapshotDetail();
            if ( ( double ) detail.get( "CurrentCompressionRatio" ) >= 1 ) {
                Assert.fail( "CurrentCompressionRatio >= 1 !" );
            }
            Assert.assertEquals( detail.get( "DictionaryCreated" ).toString(),
                    "true" );

            Assert.assertEquals( detail.get( "CompressionType" ).toString(),
                    "lzw" );
            checkData();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    private void checkData() {
        try {
            long count = this.cl
                    .getCount( "{$and:[{_id:{$gte:0}},{_id:{$lt:112}}]}" );
            Assert.assertEquals( count, 112 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @SuppressWarnings("deprecation")
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
            nameBSON.put( "Name", csName + "." + clName );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            detail = ( BSONObject ) details.get( 0 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( dataDB != null ) {
                dataDB.disconnect();
            }
        }
        return detail;
    }

    public void createCL() {
        try {
            List< String > rgNames = LzwUilts1.getDataRgNames( this.sdb );
            BSONObject option = new BasicBSONObject();
            option.put( "Group", rgNames.get( 0 ) );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            option.put( "ReplSize", 0 );
            this.cl = LzwUilts1.createCL( this.cs, this.clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass(alwaysRun = true)
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( this.clName ) ) {
                this.cs.dropCollection( this.clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }
}
