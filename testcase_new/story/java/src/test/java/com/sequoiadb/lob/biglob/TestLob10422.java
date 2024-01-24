package com.sequoiadb.lob.biglob;

import java.nio.ByteBuffer;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.BigLobUtils;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-10422:读取lob 插入lob数据，读取后比对MD5查看数据是否一致
 * @Author linsuqiang
 * @Date 2016-12-12
 * @Version 1.00
 */
public class TestLob10422 extends SdbTestBase {
    private String clName = "cl_10422";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private ByteBuffer byteBuff = null;

    public class Md5Data {
        public ObjectId oid = null;
        public String md5 = null;

        public Md5Data() {
        }

        public Md5Data( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }
    }

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( BigLobUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( BigLobUtils.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }
        createCL();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                sdb.isCollectionSpaceExist( clName );
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    @Test
    void test() {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        Md5Data md5Data = null;
        // lob sections in the same group
        md5Data = buildAndPutLob( cl );
        checkMd5( cl, md5Data );
        // lob sections in different groups
        splitCL( cl );
        md5Data = buildAndPutLob( cl );
        checkMd5( cl, md5Data );
    }

    private DBCollection createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        DBCollection cl = null;
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            BSONObject options = new BasicBSONObject();
            options = ( BSONObject ) JSON.parse(
                    "{ShardingKey:{a:1,b:-1},ShardingType:'hash',Partition:4096}" );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    private Md5Data buildAndPutLob( DBCollection cl ) {
        // build a lob
        int lobSize = 130 * 1024 * 1024;
        byte[] lobBytes = BigLobUtils.getRandomBytes( lobSize );
        // get it's md5, then insert
        Md5Data prevMd5 = new Md5Data();
        DBLob lob = null;
        try {
            lob = cl.createLob();
            lob.write( lobBytes );
            prevMd5.oid = lob.getID();
            prevMd5.md5 = BigLobUtils.getMd5( lobBytes );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }
        return prevMd5;
    }

    private void checkMd5( DBCollection cl, Md5Data prevMd5 ) {
        // check the lob by it's md5
        DBLob rLob = null;
        String curMd5 = null;
        // read lob in unit of 1k
        rLob = cl.openLob( prevMd5.oid );
        byteBuff = ByteBuffer.allocate( ( int ) rLob.getSize() );
        readLobByUnit( rLob, 1024 );
        curMd5 = BigLobUtils.getMd5( byteBuff );
        Assert.assertEquals( curMd5, prevMd5.md5,
                "the lobs md5 different(Unit: 1k)" );
        rLob.close();

        // read lob in unit of 64M
        rLob = cl.openLob( prevMd5.oid );
        readLobByUnit( rLob, 64 * 1024 * 1024 );
        curMd5 = BigLobUtils.getMd5( byteBuff );
        Assert.assertEquals( curMd5, prevMd5.md5,
                "the lobs md5 different(Unit: 64M)" );
        rLob.close();

        // read lob in unit of 128M
        rLob = cl.openLob( prevMd5.oid );
        readLobByUnit( rLob, 128 * 1024 * 1024 );
        curMd5 = BigLobUtils.getMd5( byteBuff );
        Assert.assertEquals( curMd5, prevMd5.md5,
                "the lobs md5 different(Unit: 128M)" );
        rLob.close();

    }

    private void readLobByUnit( DBLob lob, int unitSize ) {
        byteBuff.clear();
        byte[] byteUnit = new byte[ unitSize ];
        int readLen = 0;
        while ( ( readLen = lob.read( byteUnit ) ) != -1 ) {
            byteBuff.put( byteUnit, 0, readLen );
        }
        byteBuff.rewind();
    }

    private void splitCL( DBCollection cl ) {
        BSONObject cond = new BasicBSONObject();
        BSONObject endCond = new BasicBSONObject();
        cond.put( "Partition", 1024 );
        endCond.put( "Partition", 3072 );
        String sourceRGName = BigLobUtils.getSrcGroupName( sdb,
                SdbTestBase.csName, clName );
        String targetRGName = BigLobUtils.getSplitGroupName( sourceRGName );
        cl.split( sourceRGName, targetRGName, cond, endCond );

    }
}