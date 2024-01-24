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
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13390 : 指定不同lob大小truncate
 * @Author linsuqiang
 * @UpdateAuthor wuyan
 * @Date 2017-11-16
 * @UpdateDate 2019-07-23
 * @Version 1.00
 */

public class RewriteLob13390 extends SdbTestBase {
    @DataProvider(name = "truncateLengthProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : length
                // a、length等于lob大小
                new Object[] { 64 * 1024 },
                // b、length大于lob大小
                new Object[] { 64 * 1024 + 1 },
                // c、length小于lob大小（length为0，truncate所有数据；
                new Object[] { 0 },
                // c、length为1，truncate超过1byte数据；
                new Object[] { 1 },
                // c、truncate数据大小为1btye数据）
                new Object[] { 64 * 1024 - 1 }, };
    }

    private final String clName = "lobTruncate13390";
    private final int lobSize = 50 * 1024;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash',ReplSize:0}" );
        cs.createCollection( clName, clOpt );
    }

    @Test(dataProvider = "truncateLengthProvider")
    public void truncateLob( long length ) {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            byte[] writeData = RandomWriteLobUtil.getRandomBytes( lobSize );
            ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl,
                    writeData );

            cl.truncateLob( oid, length );

            // check result,when length > = lobSize,the lob is no truncate
            byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
            byte[] expData = writeData;
            long expLobSize = lobSize;
            if ( length < lobSize ) {
                expData = Arrays.copyOf( writeData, ( int ) length );
                expLobSize = length;
            }
            RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                    "lob data is wrong" );
            long actSize = getSizeByListLobs( cl, oid );
            Assert.assertEquals( actSize, expLobSize,
                    "wrong length after truncate lob" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
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
