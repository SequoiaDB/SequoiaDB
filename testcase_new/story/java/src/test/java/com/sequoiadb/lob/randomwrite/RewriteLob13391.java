package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13391 : 切分表上执行truncate lob操作
 * @Author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017-11-16
 * @UpdateDate 2019-07-23
 * @Version 1.00
 */
public class RewriteLob13391 extends SdbTestBase {
    private String clName = "truncateLobCL_13391";
    private Sequoiadb db;
    private CollectionSpace cs;
    private DBCollection dbcl;
    private final int lobSize = 1024 * 1024 * 5;

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        if ( CommLib.isStandAlone( db ) || CommLib.OneGroupMode( db ) ) {
            throw new SkipException( "less than two groups will skip!" );
        }

        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" );
        dbcl = cs.createCollection( clName, clOpt );
        String srcGroupName = RandomWriteLobUtil.getSrcGroupName( db,
                SdbTestBase.csName, clName );
        String dstGroupName = RandomWriteLobUtil.getSplitGroupName( db,
                srcGroupName );
        dbcl.split( srcGroupName, dstGroupName, 50 );
    }

    /**
     * 1、指定oid执行truncateLob操作，删除超过指定长度部分的数据，其中truncate数据切片覆盖源组和目标组
     * 2、检查操作结果（读取lob，查看lob对象长度，执行listLobs查看lobsize信息）
     * 1、执行truncateLob成功，读取lob数据为truncate后的数据一致
     * 2、执行listLobs查看lob大小为truncate操作时指定大小
     */
    @Test
    public void testLob13391() {
        byte[] writeData = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl, writeData );

        long length = 1024 * 512;
        dbcl.truncateLob( oid, length );

        byte[] expData = Arrays.copyOf( writeData, ( int ) length );
        byte[] actData = RandomWriteLobUtil.readLob( dbcl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( null != db ) {
                db.close();
            }
        }
    }
}
