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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName seqDB-13389: truncateLob时分片边界值验证
 * @Author linsuqiang
 * @Date 2017-11-16
 * @Version 1.00
 */

/*
 * 1、指定oid执行truncateLob操作，删除超过指定长度部分的数据，其中truncate 长度在切片不同位置，分别验证如下场景：
 * a、truncate长度在切片起始位置（起始1b） b、truncate长度在切片中间位置 c、truncate长度在切片末尾少1b
 * d、truncate长度刚好在切片末尾 2、检查操作结果（读取lob，查看lob对象长度，执行listLobs查看lobsize信息）
 */

public class RewriteLob13389 extends SdbTestBase {

    private final String csName = "lobTruncate13389";
    private final String clName = "lobTruncate13389";
    private final int lobPageSize = 128 * 1024; // 128k
    private final int lobMetaSize = 1 * 1024; // 1k

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash',ReplSize:0}" );
        cs.createCollection( clName, clOpt );
    }

    @DataProvider(name = "lengthProvider", parallel = true)
    public Object[][] lengthProvider() {
        Object[][] result = null;
        long firstDataPagePos = lobPageSize - lobMetaSize;
        result = new Object[][] {
                // a、truncate长度在切片起始位置（起始1b）
                new Object[] { ( long ) 1 },
                // b、truncate长度在切片中间位置
                new Object[] { ( long ) lobPageSize / 2 },
                // c、truncate长度在切片末尾少1b
                new Object[] { firstDataPagePos - 1 },
                // d、truncate长度刚好在切片末尾
                new Object[] { ( long ) firstDataPagePos } };
        return result;
    }

    @Test(dataProvider = "lengthProvider")
    public void testLob( long length ) {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            int lobSize = 2 * lobPageSize;
            byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
            ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );

            cl.truncateLob( oid, length );
            byte[] expData = Arrays.copyOfRange( data, 0, ( int ) length );

            byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
            RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                    "lob data is wrong" );

            long actSize = getSizeByListLobs( cl, oid );
            Assert.assertEquals( actSize, length,
                    "wrong length after truncate lob" );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private long getSizeByListLobs( DBCollection cl, ObjectId oid ) {
        DBCursor cursor = null;
        long lobSize = 0;
        boolean oidFound = false;
        try {
            cursor = cl.listLobs();
            while ( cursor.hasNext() ) {
                BSONObject res = cursor.getNext();
                ObjectId curOid = ( ObjectId ) res.get( "Oid" );
                if ( curOid.equals( oid ) ) {
                    lobSize = ( long ) res.get( "Size" );
                    oidFound = true;
                    break;
                }
            }
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
        if ( !oidFound ) {
            throw new RuntimeException( "no such oid" );
        }
        return lobSize;
    }
}
