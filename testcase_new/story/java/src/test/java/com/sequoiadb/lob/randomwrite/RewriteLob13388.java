package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13388 : 执行truncate lob操作
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.11.20
 * @UpdateDate 2019.07.22
 * @version 1.10
 */

public class RewriteLob13388 extends SdbTestBase {
    private String clName = "truncateLobCL_13388";
    private Sequoiadb db;
    private CollectionSpace cs;
    private DBCollection dbcl;

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        dbcl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{ShardingKey:{\"_id\":1}}" ) );
    }

    /**
     * 1、指定oid执行truncateLob操作，删除超过指定长度部分的数据
     * 2、检查操作结果（读取lob，查看lob对象长度，执行listLobs查看lobsize信息）
     * 1、执行truncateLob成功，读取lob数据为truncate后的数据一致
     * 2、执行listLobs查看lob大小为truncate操作时指定大小
     */
    @Test
    public void testLob13388() {
        int writeSize = 1024 * 5;
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        long length = 100;
        dbcl.truncateLob( oid, length );

        // check result
        byte[] actData = RandomWriteLobUtil.readLob( dbcl, oid );
        byte[] expData = Arrays.copyOf( lobBuff, ( int ) length );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData );
        try ( DBCursor listLob = dbcl.listLobs()) {
            while ( listLob.hasNext() ) {
                BSONObject obj = listLob.getNext();
                long actLobSize = ( Long ) obj.get( "Size" );
                Assert.assertEquals( actLobSize, length );
            }
        }
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
}
