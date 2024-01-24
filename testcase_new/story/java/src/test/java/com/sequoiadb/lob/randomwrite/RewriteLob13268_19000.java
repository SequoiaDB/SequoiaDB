package com.sequoiadb.lob.randomwrite;

import static com.sequoiadb.lob.utils.RandomWriteLobUtil.assertByteArrayEqual;
import static com.sequoiadb.lob.utils.RandomWriteLobUtil.createEmptyLob;
import static com.sequoiadb.lob.utils.RandomWriteLobUtil.getRandomBytes;
import static com.sequoiadb.lob.utils.RandomWriteLobUtil.readLob;

import java.util.ArrayList;
import java.util.Arrays;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description: seqDB-13268:普通表并发加锁写lob seqDB-19000 :: 版本: 1 :: 主子表并发加锁写lob
 * @author laojingtang
 * @Date 2017.12.01
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 */
public class RewriteLob13268_19000 extends SdbTestBase {
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "writelob13268";
    private String mainCLName = "mainCL_19000";
    private String subCLName = "subCL_19000";

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is subCLName
                // testcase:13268
                new Object[] { clName },
                // testcase:19000
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setupClass() {
        db = new Sequoiadb( coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
        if ( !CommLib.isStandAlone( db ) ) {
            LobSubUtils.createMainCLAndAttachCL( db, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    /**
     * 1、共享模式下，多个连接多线程并发如下操作: (1)打开已存在lob对象，seek指定偏移范围，执行lock锁定数据段，向锁定数据段写入lob
     * 多个并发线程中锁定数据段范围不冲突 2、读取lob，检查操作结果
     * 1、所有线程写入lob成功，查询lob信息按指定位置写入数据，且写入数据信息正确（比较MD5值）
     */
    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) throws InterruptedException {
        if ( CommLib.isStandAlone( db ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        final ObjectId id = createEmptyLob( cl );

        List< byte[] > datas = new ArrayList<>( 10 );
        List< DbLobWriteTask > tasks = new ArrayList<>( 10 );
        int step = 1024;
        for ( int i = 0; i < 10; i++ ) {
            byte[] data = getRandomBytes( step );
            datas.add( data );
            tasks.add( new DbLobWriteTask( SdbTestBase.csName, clName, id,
                    i * step, data ) );
        }

        for ( DbLobWriteTask task : tasks )
            task.start();
        for ( DbLobWriteTask task : tasks )
            task.join();
        for ( DbLobWriteTask task : tasks )
            Assert.assertTrue( task.isTaskSuccess(), task.getErrorMsg() );

        byte[] actualData = readLob( cl, id );
        for ( int i = 0; i < 10; i++ ) {
            assertByteArrayEqual(
                    Arrays.copyOfRange( actualData, i * step, i * step + step ),
                    datas.get( i ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }
}
