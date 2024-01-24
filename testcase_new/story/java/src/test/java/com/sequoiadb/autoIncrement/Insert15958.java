package com.sequoiadb.autoIncrement;

import org.bson.BSONObject;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-15958：同一个coord不指定自增字段并发插入 预置条件:集合已存在，且已存在自增字段，自增字段上创建唯一索引
 *                                           测试步骤：连接同一个coord节点，不指定自增字段并发插入记录
 *                                           预期结果：记录插入成功，自增字段值唯一且递增，值正确
 * @Author zhaoyu
 * @Date 2018-11-01
 * @Version 1.00
 */
public class Insert15958 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace scs = null;
    private DBCollection scl = null;
    private String clName = "cl_15958";
    private String indexName = "idIndex";
    private int threadNum = 10;
    private int threadInsertNum = 10000;
    private long currentValue = 1;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        scs = sdb.getCollectionSpace( csName );
        scl = scs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{AutoIncrement:{Field:\"id\"}}" ) );
        scl.createIndex( indexName, "{id:1}", true, true );
    }

    @AfterClass
    public void tearDown() {
        scs.dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() {
        InsertThread insertThread = new InsertThread();
        insertThread.start( threadNum );
        if ( !( insertThread.isSuccess() ) ) {
            Assert.fail( insertThread.getErrorMsg() );
        }

        AutoIncrementUtils.checkResult( scl, threadInsertNum * threadNum,
                currentValue );
    }

    private class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < threadInsertNum; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON
                            .parse( "{a:" + i + "}" );
                    cl.insert( obj );
                }
            }

        }
    }

}
