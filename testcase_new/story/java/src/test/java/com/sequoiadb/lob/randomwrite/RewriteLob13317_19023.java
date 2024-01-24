package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName seqDB-13317: 加锁写lob过程中执行切分 seqDB-19023 主子表加锁写lob过程中执行切分
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、共享模式下，并发执行如下操作 (1)打开已存在lob对象，seek指定偏移范围，执行lock锁定数据段，向锁定数据段写入lob，
 * 重复多次加锁不同范围写lob （2）执行切分操作（异步切分），其中切分过程中需要覆盖元数据片迁移到目标组场景 2、读取lob，检查操作结果
 */

public class RewriteLob13317_19023 extends SdbTestBase {

    private final String csName = "writelob13317";
    private final String clName = "writelob13317";
    private final String mainCLName = "mainCL19023";
    private final String subCLName = "subCL19023";
    private final int lobPageSize = 16 * 1024; // 16k

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is csName, clName
                // testcase:13318
                new Object[] { clName, clName },
                // testcase:19019
                new Object[] { mainCLName, subCLName } };
    }

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
        cs.createCollection( clName, clOpt );
        LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String mainCLName, String subCLName ) {
        DBCollection mainCL = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        DBCollection subCL = sdb.getCollectionSpace( csName )
                .getCollection( subCLName );

        int lobSize = 2 * 1024 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( mainCL, data );
        byte[] expData = data;

        LobPart partA = new LobPart( 0, 256 * 1024 );
        LobPart partB = new LobPart( 256 * 1024, 768 * 1024 );
        LobPart partC = new LobPart( 1024 * 1024, 512 * 1024 );

        String srcGroupName = RandomWriteLobUtil.getSrcGroupName( sdb, csName,
                subCLName );
        String dstGroupName = RandomWriteLobUtil.getSplitGroupName( sdb,
                srcGroupName );
        long taskId = 0;
        // TODO:1、切分范围没有检查
        taskId = subCL.splitAsync( srcGroupName, dstGroupName, 50 );

        try ( DBLob lob = mainCL.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lockAndSeekAndWriteLob( lob, partA );
            lockAndSeekAndWriteLob( lob, partB );
            lockAndSeekAndWriteLob( lob, partC );
        }

        expData = updateExpData( expData, partA );
        expData = updateExpData( expData, partB );
        expData = updateExpData( expData, partC );

        long[] taskIdArr = { taskId };
        sdb.waitTasks( taskIdArr );

        byte[] actData = RandomWriteLobUtil.readLob( mainCL, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );

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

    private void lockAndSeekAndWriteLob( DBLob lob, LobPart part ) {
        lob.lockAndSeek( part.getOffset(), part.getLength() );
        lob.write( part.getData() );
    }

    private byte[] updateExpData( byte[] expData, LobPart part ) {
        return RandomWriteLobUtil.appendBuff( expData, part.getData(),
                part.getOffset() );
    }

}
