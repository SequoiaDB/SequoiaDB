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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13319: 并发加锁写lob过程中执行truncate seqDB-19020
 *           主子表并发加锁写lob过程中执行truncate
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、共享模式下，多个连接多线程并发如下操作: (1)打开已存在lob对象，seek指定偏移范围，执行lock锁定数据段，向锁定数据段写入lob，
 * 其中多个线程锁定不同数据段 （2）执行truncate操作 2、读取lob，检查操作结果
 */

public class RewriteLob13319_19020 extends SdbTestBase {

    private final String csName = "writelob13319";
    private final String clName = "writelob13319";
    private final String mainCLName = "mainCL19020";
    private final String subCLName = "subCL19020";
    private final int lobPageSize = 32 * 1024; // 32k
    private final int threadNum = 4;
    private final int writeSizePerThread = 8 * lobPageSize;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is csName, clName
                // testcase:13319
                new Object[] { clName },
                // testcase:19020
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
        TruncateThread trunThrd = new TruncateThread( csName, clName );
        for ( int i = 0; i < threadNum; ++i ) {
            WriteLobThread wLobThrd = new WriteLobThread( csName, clName, oid,
                    parts.get( i ) );
            wLobThrds.add( wLobThrd );
        }

        // start
        trunThrd.start();
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            wLobThrd.start();
        }

        // join
        Assert.assertTrue( trunThrd.isSuccess(), trunThrd.getErrorMsg() );
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            Assert.assertTrue( wLobThrd.isSuccess(), wLobThrd.getErrorMsg() );
        }

        // check truncate ok
        Assert.assertFalse( isLobExist( cl, oid ),
                "lob still exist after truncate!" );
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

        public WriteLobThread( String csName, String clName, ObjectId oid,
                LobPart part ) {
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
                if ( errCode != -4 && // -4: file not exist
                        errCode != -321 && // -321: cl truncate
                        errCode != -268 ) { // -268: lob sequence not exist
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

    private class TruncateThread extends SdbThreadBase {

        private String csNamet;
        private String clNamet;

        public TruncateThread( String csName, String clName ) {
            this.csNamet = csName;
            this.clNamet = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( this.csNamet )
                        .getCollection( this.clNamet );
                cl.truncate();
            }
        }
    }

    private boolean isLobExist( DBCollection cl, ObjectId oid ) {
        DBCursor cursor = cl.listLobs();
        boolean oidFound = false;
        while ( cursor.hasNext() ) {
            BSONObject currRec = cursor.getNext();
            ObjectId currOid = ( ObjectId ) currRec.get( "Oid" );
            if ( currOid.equals( oid ) ) {
                oidFound = true;
                break;
            }
        }
        cursor.close();
        return oidFound;
    }

}
