package com.sequoiadb.lob.subcl;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-19070 : 并发主表挂载子表和子表读写删lob
 * @author wuyan
 * @Date 2019.8.26
 * @version 1.0
 */

public class LobSubCL19070 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private DBCollection mainCL = null;
    private String mainCLName = "mainCL_19070";
    private String subCLName = "subCL_19070";
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;
    private ObjectId lobOid;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        // create maincl
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        mainCL = cs.createCollection( mainCLName, options );
        // create subcl
        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "datetime", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "AutoSplit", true );
        cs.createCollection( subCLName, clOptions );

        lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor();
        thread.addWorker( new AttachCLThread() );
        thread.addWorker( new PutLobThread() );
        thread.run();

        Assert.assertTrue( isAttachMainCL( sdb, csName, subCLName ),
                "check sub cl attach result" );
        readLobFromCL( mainCL, lobOid );
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class AttachCLThread {
        @ExecuteOrder(step = 1)
        private void attachCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainCLName );
                // 随机等待1000ms，在lob操作不同阶段attachCL
                try {
                    Thread.sleep( new Random().nextInt( 1000 ) );
                } catch ( InterruptedException e ) {
                }

                BSONObject bound = new BasicBSONObject();
                bound.put( "LowBound",
                        new BasicBSONObject( "date", new MinKey() ) );
                bound.put( "UpBound",
                        new BasicBSONObject( "date", new MaxKey() ) );
                mainCL.attachCollection( SdbTestBase.csName + "." + subCLName,
                        bound );
            }
        }
    }

    private class PutLobThread {
        @ExecuteOrder(step = 1)
        private void putLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName );
                ObjectId lobId = RandomWriteLobUtil.createAndWriteLob( dbcl,
                        lobBuff );
                // read lob
                readLobFromCL( dbcl, lobId );
                // delete lob
                dbcl.removeLob( lobId );
            }
        }

        // 插入lob，子表挂载成功后该lob可通过主表读取
        @ExecuteOrder(step = 1)
        private void putLob1() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName );
                lobOid = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );
            }
        }
    }

    private boolean isAttachMainCL( Sequoiadb db, String csName,
            String subCLName ) {
        String clFullName = csName + "." + subCLName;
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", clFullName ), null, null );
        if ( cursor.hasNext() ) {
            BasicBSONObject subCLInfo = ( BasicBSONObject ) cursor.getNext();
            return subCLInfo.containsField( "MainCLName" );
        }
        cursor.close();
        return false;
    }

    private void readLobFromCL( DBCollection dbcl, ObjectId lobId ) {
        try ( DBLob lob = dbcl.openLob( lobId )) {
            byte[] actualBuff = new byte[ ( int ) lob.getSize() ];
            lob.read( actualBuff );
            RandomWriteLobUtil.assertByteArrayEqual( actualBuff, lobBuff );
        }
    }
}
