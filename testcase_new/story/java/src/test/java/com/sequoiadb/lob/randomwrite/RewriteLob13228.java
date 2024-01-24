package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13228 : seek偏移写lob
 * @author laojingtang
 * @UpdateAuthor wangkexin
 * @Date 2017.11.22
 * @UpdateDate 2018.08.15
 * @version 1.10
 */
public class RewriteLob13228 extends SdbTestBase {
    @DataProvider(name = "testLob13228DataProvider")
    public static Object[][] testLob13228DataProvider() {
        return new Object[][] {
                // 同一个数据页内偏移写
                { 200 * 1024, 10 * 1024, DBLob.SDB_LOB_SEEK_SET },
                { 100 * 1024, 20 * 1024, DBLob.SDB_LOB_SEEK_CUR },
                { 100 * 1024, 50 * 1024, DBLob.SDB_LOB_SEEK_END },

                // 跨多个数据页偏移写
                { 1024 * 1024, 100 * 1024, DBLob.SDB_LOB_SEEK_SET },
                { 1024 * 1024, 50 * 1024, DBLob.SDB_LOB_SEEK_CUR },
                { 1024 * 1024, 40 * 1024, DBLob.SDB_LOB_SEEK_END }, };
    }

    private Sequoiadb db = null;
    private DBCollection dbcl = null;
    private CollectionSpace cs = null;
    private String clName = "lobcl_13228";

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        dbcl = cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
    }

    @Test(dataProvider = "testLob13228DataProvider")
    public void testLob13228( int appendDataSize, int seekSize, int seekType ) {
        // 第一种情况：创建lob后先向lob中写数据，再进行偏移写操作
        testLob13228a( appendDataSize, seekSize, seekType );

        // 第二种情况：创建lob后直接进行偏移写操作
        testLob13228b( appendDataSize, seekSize, seekType );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private void testLob13228a( int appendDataSize, int seekSize,
            int seekType ) {
        // 设置初始数据大小100kb
        int initDataSize = 100 * 1024;

        ObjectId id = ObjectId.get();
        DBLob lob = dbcl.createLob( id );

        byte[] initData = RandomWriteLobUtil.getRandomBytes( initDataSize );
        String initMd5 = RandomWriteLobUtil.getMd5( initData );

        byte[] appendData = RandomWriteLobUtil.getRandomBytes( appendDataSize );
        String appendMd5 = RandomWriteLobUtil.getMd5( appendData );

        lob.write( initData );

        lob.seek( seekSize, seekType );
        lob.write( appendData );
        lob.close();

        // 偏移写初始位置
        int appendPosition;
        byte[] actual = RandomWriteLobUtil.readLob( dbcl, id );

        if ( seekType == DBLob.SDB_LOB_SEEK_CUR ) {
            appendPosition = seekSize + initDataSize;
        } else if ( seekType == DBLob.SDB_LOB_SEEK_END ) {
            appendPosition = initDataSize - seekSize;
        } else {
            appendPosition = seekSize;
        }

        if ( appendPosition > initDataSize ) {
            String previnitMd5 = RandomWriteLobUtil
                    .getMd5( Arrays.copyOfRange( actual, 0, initDataSize ) );
            Assert.assertEquals( previnitMd5, initMd5 );

            String prevappendMd5 = RandomWriteLobUtil
                    .getMd5( Arrays.copyOfRange( actual, appendPosition,
                            appendPosition + appendDataSize ) );
            Assert.assertEquals( prevappendMd5, appendMd5 );

            // 比较lob size 信息是否正确
            Assert.assertEquals( lob.getSize(),
                    initDataSize + seekSize + appendDataSize );
        } else {
            String prevappendMd5 = RandomWriteLobUtil
                    .getMd5( Arrays.copyOfRange( actual, appendPosition,
                            appendPosition + appendDataSize ) );
            Assert.assertEquals( prevappendMd5, appendMd5 );

            // 比较lob size 信息是否正确
            if ( seekType == DBLob.SDB_LOB_SEEK_SET ) {
                Assert.assertEquals( lob.getSize(), appendDataSize + seekSize );
            } else {
                Assert.assertEquals( lob.getSize(),
                        initDataSize - seekSize + appendDataSize );
            }
        }
    }

    private void testLob13228b( int appendDataSize, int seekSize,
            int seekType ) {
        ObjectId id = ObjectId.get();
        DBLob lob = dbcl.createLob( id );

        byte[] appendData = RandomWriteLobUtil.getRandomBytes( appendDataSize );
        String appendMd5 = RandomWriteLobUtil.getMd5( appendData );

        if ( seekType == DBLob.SDB_LOB_SEEK_END ) {
            lob.seek( 0, seekType );
        } else {
            lob.seek( seekSize, seekType );
        }
        lob.write( appendData );
        lob.close();

        // check the result
        byte[] actual = RandomWriteLobUtil.readLob( dbcl, id );
        if ( seekType != DBLob.SDB_LOB_SEEK_END ) {
            String previnitMd5 = RandomWriteLobUtil.getMd5( Arrays.copyOfRange(
                    actual, seekSize, seekSize + appendDataSize ) );
            Assert.assertEquals( appendMd5, previnitMd5 );
            Assert.assertEquals( lob.getSize(), seekSize + appendDataSize );
        } else {
            String previnitMd5 = RandomWriteLobUtil.getMd5( actual );
            Assert.assertEquals( appendMd5, previnitMd5 );
            Assert.assertEquals( lob.getSize(), appendDataSize );
        }
    }
}
