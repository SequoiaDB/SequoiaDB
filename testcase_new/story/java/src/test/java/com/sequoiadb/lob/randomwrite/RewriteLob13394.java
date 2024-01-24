package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13394:并发写和truncate lob
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.11.20
 * @UpdateDate 2019.07.22
 * @version 1.10
 */
public class RewriteLob13394 extends SdbTestBase {
    private String clName = "lob_13394";
    private Sequoiadb db;
    private CollectionSpace cs;
    private DBCollection dbcl;
    private final int lobSize = 1024;
    private final byte[] _randomDatas = RandomWriteLobUtil
            .getRandomBytes( lobSize );

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( csName );
        dbcl = cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
    }

    /**
     * 1、多个连接多线程并发如下操作： （1）创建lob，写入lob数据 （2）truncate删除lob（lob写入过程中并发truncate操作）
     * 2、检查写入和truncatelob结果 1、写入lob成功，truncate删除lob失败，返回对应错误信息，查询写入lob数据正确
     */
    @Test
    public void testLob13394() throws InterruptedException {
        final ObjectId id = ObjectId.get();
        final AtomicBoolean canTruncate = new AtomicBoolean( false );

        DbClOperateTask createDbClTask = new DbClOperateTask(
                SdbTestBase.csName, clName ) {
            @Override
            protected void exec() throws Exception {
                try ( DBLob lob = this.dbcl.createLob( id )) {
                    canTruncate.set( true );
                    lob.write( _randomDatas );
                }
            }
        };

        DbClOperateTask truncateLob = new DbClOperateTask( SdbTestBase.csName,
                clName ) {
            @Override
            protected void exec() throws Exception {
                while ( !canTruncate.get() ) {
                    Thread.sleep( 100 );
                }
                this.dbcl.truncateLob( id, 100 );
            }
        };

        createDbClTask.start();
        truncateLob.start();
        createDbClTask.join();
        truncateLob.join();

        String lobErrMsg = "lob id: " + id.toString();
        Assert.assertTrue( createDbClTask.isTaskSuccess(),
                createDbClTask.getErrorMsg() );

        if ( truncateLob.isTaskSuccess() )
            RandomWriteLobUtil.assertByteArrayEqual(
                    RandomWriteLobUtil.readLob( dbcl, id ),
                    Arrays.copyOf( _randomDatas, 100 ), lobErrMsg );
        else {
            Assert.assertEquals( truncateLob.getSdbErrCode(),
                    SDBError.SDB_LOB_IS_IN_USE.getErrorCode(), lobErrMsg );
            RandomWriteLobUtil.assertByteArrayEqual(
                    RandomWriteLobUtil.readLob( dbcl, id ), _randomDatas,
                    lobErrMsg );
        }
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        db.close();
    }
}
