package com.sequoiadb.lob.randomwrite;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13606:并发加锁写lob过程中drop cl seqDB-19021 主子表并发加锁写lob过程中drop cl
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * (1)打开已存在lob对象，seek指定偏移范围，执行lock锁定数据段，向锁定数据段写入lob，
 * 其中多个线程锁定不同数据段，锁定数据段范围较大（如每个数据段锁定10G大小） （2）写lob过程中执行drop cl操作 2、读取lob，检查操作结果
 */

public class RewriteLob13606_19021 extends SdbTestBase {

    private final String csName = "writelob13606";
    private final String clName = "writelob13606";
    private final String mainCLName = "mainCL19021";
    private final String subCLName = "subCL19021";
    private final int lobPageSize = 32 * 1024; // 32k
    private final int threadNum = 4;
    private final int writeSizePerThread = 8 * lobPageSize;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is csName, clName
                // testcase:13606
                new Object[] { clName },
                // testcase:19021
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        cs.createCollection( clName, clOpt );

        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                    subCLName );
        }

    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        int lobSize = 1 * 1024 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        List< LobPart > parts = getParts( threadNum, writeSizePerThread );
        List< WriteLobThread > wLobThrds = new ArrayList< WriteLobThread >();

        // init
        DropCLThread trunThrd = new DropCLThread( clName );
        for ( int i = 0; i < threadNum; ++i ) {
            WriteLobThread wLobThrd = new WriteLobThread( clName, oid,
                    parts.get( i ) );
            wLobThrds.add( wLobThrd );
        }

        // start
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            wLobThrd.start();
        }
        trunThrd.start();

        // join
        Assert.assertTrue( trunThrd.isSuccess(), trunThrd.getErrorMsg() );
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            Assert.assertTrue( wLobThrd.isSuccess(), wLobThrd.getErrorMsg() );
        }

        // check truncate ok
        Assert.assertFalse( cs.isCollectionExist( clName ),
                "cl still exist after drop!" );

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private class WriteLobThread extends SdbThreadBase {

        private String clNamet;
        private ObjectId oid = null;
        private LobPart part = null;

        public WriteLobThread( String clName, ObjectId oid, LobPart part ) {
            this.clNamet = clName;
            this.oid = oid;
            this.part = part;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( this.clNamet );
                DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE );

                lob.lockAndSeek( part.getOffset(), part.getLength() );
                lob.write( part.getData() );
                lob.close();
            } catch ( BaseException e ) {
                int errCode = e.getErrorCode();
                if ( errCode != -23 ) { // -23: SDB_DMS_NOTEXIST
                    throw e;
                }
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

    private List< LobPart > getParts( int partNum, int partSize ) {
        List< LobPart > parts = new ArrayList< LobPart >();
        for ( int i = 0; i < partNum; ++i ) {
            LobPart part = new LobPart( i * partSize, partSize );
            parts.add( part );
        }
        return parts;
    }

    private class DropCLThread extends SdbThreadBase {

        private String clNamet;

        public DropCLThread( String clName ) {
            this.clNamet = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( csName ).dropCollection( this.clNamet );
            }
        }
    }

}
