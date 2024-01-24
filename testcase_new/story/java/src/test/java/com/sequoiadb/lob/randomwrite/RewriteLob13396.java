package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
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
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13396 : truncate lob过程中执行切分
 * @Author linsuqiang
 * @Date 2017-11-16
 * @Version 1.00
 */

/*
 * 1、多个连接多线程并发如下操作： （1）truncate删除lob（删除lob数据较大） （2）执行切分操作（切分覆盖元数据片在源组和目标组场景）
 * 2、检查写入和truncatelob结果
 */

public class RewriteLob13396 extends SdbTestBase {

    private final String csName = "lobTruncate13396";
    private final String clName = "lobTruncate13396";
    private final int lobPageSize = 4 * 1024; // 32k
    private final int lobSize = 100 * 1024 * 1024;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    private String srcGroupName = null;
    private String dstGroupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "no groups to split" );
        }

        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        cl = cs.createCollection( clName, clOpt );

        srcGroupName = RandomWriteLobUtil.getSrcGroupName( sdb, csName,
                clName );
        dstGroupName = RandomWriteLobUtil.getSplitGroupName( sdb,
                srcGroupName );

    }

    @Test
    public void truncateWhenSplit() {

        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );

        SplitThread splitThrd = new SplitThread( srcGroupName, dstGroupName );
        TruncateThread trunThrd = new TruncateThread( oid );

        splitThrd.start();
        trunThrd.start();
        Assert.assertTrue( splitThrd.isSuccess(), splitThrd.getErrorMsg() );
        Assert.assertTrue( trunThrd.isSuccess(), trunThrd.getErrorMsg() );

        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        Assert.assertEquals( actData.length, 0, "wrong lob length" );

        checkSplitOk();
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

    private class SplitThread extends SdbThreadBase {
        private String srcGroupName = null;
        private String dstGroupName = null;

        public SplitThread( String srcGroupName, String dstGroupName ) {
            this.srcGroupName = srcGroupName;
            this.dstGroupName = dstGroupName;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroupName, dstGroupName, 50 );
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

    private class TruncateThread extends SdbThreadBase {
        private ObjectId oid = null;

        public TruncateThread( ObjectId oid ) {
            this.oid = oid;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncateLob( oid, 0 );
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

    private void checkSplitOk() {
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        BSONObject rec = cursor.getNext();
        cursor.close();

        BasicBSONList cataInfo = ( BasicBSONList ) rec.get( "CataInfo" );
        Assert.assertEquals( 2, cataInfo.size() );
    }
}
