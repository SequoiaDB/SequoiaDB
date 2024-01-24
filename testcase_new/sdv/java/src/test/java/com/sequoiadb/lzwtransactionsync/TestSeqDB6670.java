package com.sequoiadb.lzwtransactionsync;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lzwtransaction.LzwTransUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 1、日志满导致事务回滚的场景，比如：日志文件大小配置小点，开启事务，大批量灌数据
 * 
 * @author chensiqin
 * @Date 2016-12-16
 */
public class TestSeqDB6670 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl6670";
    private String rgName = "rg6670";
    private int port1;
    private int port2;
    private int port3;
    private boolean runSuccess = false;

    @BeforeClass()
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standAlone." );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "skip one group mode." );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        port1 = SdbTestBase.reservedPortBegin + 670;
        port2 = SdbTestBase.reservedPortBegin + 680;
        port3 = SdbTestBase.reservedPortBegin + 690;
    }

    /**
     * 1、CL压缩类型为lzw，开启事务，对CL做增删改查数据 2、日志爆满，自动回滚 3、检查返回结果，并检查数据压缩情况
     * 
     * @throws Exception
     */
    @Test()
    public void test() throws Exception {
        // 1. 创建新组及3个数据节点
        if ( sdb.isReplicaGroupExist( rgName ) ) {
            sdb.removeReplicaGroup( rgName );
        }

        // 取coord节点的全局事务及mvcc配置，加入到节点配置上，避免mvcc分支，事务执行报-6的错误
        BSONObject configure = new BasicBSONObject();
        BSONObject transConfig = getTransConfig( sdb, "data" );
        configure.putAll( transConfig );
        configure.put( "logfilenum", 5 );
        configure.put( "transactionon", true );
        configure.put( "diaglevel", 5 );

        String hostName = sdb.getReplicaGroup( "SYSCatalogGroup" ).getMaster()
                .getHostName();
        ReplicaGroup rg = sdb.createReplicaGroup( rgName );
        rg.createNode( hostName, port1,
                SdbTestBase.reservedDir + "/data/" + port1 + "/", configure );
        rg.createNode( hostName, port2,
                SdbTestBase.reservedDir + "/data/" + port2 + "/", configure );
        rg.createNode( hostName, port3,
                SdbTestBase.reservedDir + "/data/" + port3 + "/", configure );
        rg.start();

        // 2.集合创建在新建的组上
        BSONObject option = new BasicBSONObject();
        option.put( "Group", rgName );
        option.put( "Compressed", true );
        option.put( "CompressionType", "lzw" );
        cl = LzwTransUtils.createCL( cs, clName, option );

        // 3.插入数据，使cl存在已被压缩的记录
        LzwTransUtils util = new LzwTransUtils();
        util.insertData( cl, 0, 99, 1024 * 1024 );
        util.insertData( cl, 99, 109, 1024 * 1024 );
        BSONObject bObject = getSnapshotDetail();
        while ( !"true"
                .equals( bObject.get( "DictionaryCreated" ).toString() ) ) {
            Thread.sleep( 10 * 1000 );
            bObject = getSnapshotDetail();
        }

        // 4.继续插入数据，数据会被压缩，之后压缩率会小于1
        util.insertData( cl, 109, 115, 1024 );
        BSONObject before = getSnapshotDetail();
        if ( ( double ) before.get( "CurrentCompressionRatio" ) >= 1 ) {
            Assert.fail( "CurrentCompressionRatio >= 1 !" );
        }
        Assert.assertEquals( before.get( "DictionaryCreated" ).toString(),
                "true" );

        // 5.开启事务，对cl做增删改查操作,将日志写满
        try {
            sdb.beginTransaction();
            cl.delete( "{_id:{$et:114}}", "{'':'_id'}" );
            for ( int i = 1; i <= 40000; i++ ) {
                util.insertData( cl, 115, 116, 1024 );
                cl.delete( "{_id:115}" );
            }
            Assert.fail( "need throw an error." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -203 );
        }

        // 6.校验集合中的数据及主备节点一致性
        Assert.assertTrue( TransUtil.isCLConsistency( cl ) );
        Assert.assertEquals(
                cl.getCount( "{$and:[{_id:{$gte:0}},{_id:{$lt:115}}]}" ), 115 );
        Sequoiadb slaveData = null;
        try {
            slaveData = sdb.getReplicaGroup( rgName ).getSlave().connect();
            DBCollection slaveCL = slaveData.getCollectionSpace( csName )
                    .getCollection( clName );
            Assert.assertEquals( slaveCL.getCount(
                    "{$and:[{_id:{$gte:0}},{_id:{$lt:115}}]}" ), 115 );
        } finally {
            slaveData.close();
        }

        // 7.回滚后记录跟开启事务前数据一致且记录压缩状态一致
        BSONObject after = getSnapshotDetail();
        Assert.assertEquals( before.get( "CompressionType" ).toString(),
                after.get( "CompressionType" ).toString() );
        Assert.assertEquals( before.get( "DictionaryCreated" ).toString(),
                after.get( "DictionaryCreated" ).toString() );
        if ( ( double ) after.get( "CurrentCompressionRatio" ) >= 1 ) {
            Assert.fail( "CurrentCompressionRatio >= 1 !" );
        }

        runSuccess = true;
    }

    public BSONObject getSnapshotDetail() {
        BSONObject detail = null;
        Sequoiadb dataDB = null;
        try {
            detail = new BasicBSONObject();
            String url = LzwTransUtils.getGroupIPByGroupName( sdb, rgName );
            dataDB = new Sequoiadb( url, "", "" );
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", csName + "." + clName );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            detail = ( BSONObject ) details.get( 0 );
        } finally {
            if ( dataDB != null ) {
                dataDB.close();

            }
        }
        return detail;
    }

    public BSONObject getTransConfig( Sequoiadb db, String role ) {
        DBCursor configCur = db.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS,
                "{role:'" + role + "'}", "{globtranson:'',mvccon:''}", "" );
        BSONObject transConfig = configCur.getNext();
        configCur.close();
        return transConfig;
    }

    @AfterClass()
    public void tearDown() {
        if ( runSuccess ) {
            cs.dropCollection( clName );
            if ( sdb.isReplicaGroupExist( rgName ) ) {
                sdb.removeReplicaGroup( rgName );
            }
        }
        sdb.close();
    }

}
