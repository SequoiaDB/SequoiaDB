package com.sequoiadb.lob.randomwrite;

import static com.sequoiadb.lob.utils.RandomWriteLobUtil.assertByteArrayEqual;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

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
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13271:并发读lob，其中读数据范围连续 seqDB-19013 :: 版本: 1 ::
 *              主子表并发读lob，其中读数据范围连续
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.11.20
 * @UpdateDate 2018.08.26
 * @version 1.10
 */
public class RewriteLob13271_19013 extends SdbTestBase {
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "lob_13271";
    private String mainCLName = "mainCL_19013";
    private String subCLName = "subCL_19013";

    private static Map< Integer, byte[] > randomBytesMap = new HashMap<>();

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is subCLName
                // testcase:13271
                new Object[] { clName },
                // testcase:19013
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );

        if ( !CommLib.isStandAlone( db ) ) {
            LobSubUtils.createMainCLAndAttachCL( db, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    /**
     * 1、多个连接多线程并发如下操作: (1)打开已存在lob对象，seek指定偏移范围，执行read读取指定范围内数据
     * 多个并发线程中指定数据范围连续，如线程1读数据范围为1-3、线程2读数据范围为4-5、线程3锁定数据范围为5-9 2、检查操作结果
     * 1、所有线程读lob成功，查询对应范围内lob信息和实际写入数据一致（比较MD5值）
     */
    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) throws InterruptedException {
        if ( CommLib.isStandAlone( db ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        int lobsize = 1024 * 1024 * 20;
        byte[] expectBytes = getRandomBytes( lobsize );
        DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBLob lob = dbcl.createLob();
        final ObjectId oid = lob.getID();
        lob.write( expectBytes );
        lob.close();

        List< DbLobReadTask > lobTasks = new ArrayList<>( 10 );

        final int step = lobsize / 10;
        for ( int i = 0; i < 10; i++ ) {
            final int begin = i * step;
            lobTasks.add( new DbLobReadTask( SdbTestBase.csName, clName, begin,
                    step, oid ) );
        }

        for ( DbLobReadTask lobTask : lobTasks )
            lobTask.start();
        for ( DbLobReadTask lobTask : lobTasks )
            lobTask.join();
        for ( DbLobReadTask lobTask : lobTasks ) {
            Assert.assertTrue( lobTask.isTaskSuccess(), lobTask.getErrorMsg() );
            int b = lobTask.getBegin();
            int length = lobTask.getLength();
            byte[] expect = Arrays.copyOfRange( expectBytes, b, b + length );
            byte[] actual = lobTask.getResult();
            assertByteArrayEqual( actual, expect );
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
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private byte[] getRandomBytes( int size ) {
        if ( randomBytesMap.containsKey( size ) )
            return randomBytesMap.get( size );
        else {
            byte[] b = RandomWriteLobUtil.getRandomBytes( size );
            randomBytesMap.put( size, b );
            return b;
        }
    }

}
