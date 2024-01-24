package com.sequoiadb.autoIncrement;

/**
 * @FileName:seqDB-15962： 不指定自增字段插入与创建自增字段并发 
 * 预置条件:集合已存在
 * 测试步骤：不指定自增字段插入的同时，创建自增字段，指定AcquireSize为1
 * 预期结果：记录插入成功，自增字段创建成功，自增字段创建前，记录无自增字段值，自增字段创建后，记录新增自增字段值，
 *       且值唯一且递增，值正确
 * @Author zhaoxiaoni
 * @Date 2018-11-15
 * @Version 1.00
 */
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

public class Insert15962 extends SdbTestBase {
    private Sequoiadb commSdb = null;
    private String clName = "cl_15962";
    private String fieldName = "id";
    private int threadInsertNum = 10000;
    private long increment = 1;
    private long startValue = 1;

    @BeforeClass
    public void setUp() {
        commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        DBCollection cl = commSdb.getCollectionSpace( csName )
                .createCollection( clName );
    }

    @Test
    public void test() {
        Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        InsertThread insertThread = new InsertThread();
        insertThread.start();

        CreateThread createThread = new CreateThread();
        createThread.start();

        if ( !( insertThread.isSuccess() && createThread.isSuccess() ) ) {
            Assert.fail(
                    insertThread.getErrorMsg() + createThread.getErrorMsg() );
        }

        // check records count
        long count = ( long ) cl.getCount();
        Assert.assertEquals( count, threadInsertNum );

        DBCursor cursor = cl.query( "{id:{$exists:1}}", null, "{id:1}", null );
        long value = startValue;
        while ( cursor.hasNext() ) {
            long cursorValue = ( long ) cursor.getNext().get( "id" );
            if ( cursorValue != value ) {
                Assert.fail( "records error!" );
            }
            value = increment + value;
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = commSdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
                ;
            }
        }
    }

    private class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" ) ;) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < threadInsertNum; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON
                            .parse( "{a:" + i + "}" );
                    cl.insert( obj );
                }
            }
        }
    }

    private class CreateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" ) ;) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createAutoIncrement( ( BSONObject ) JSON.parse(
                        "{Field:'" + fieldName + "', AcquireSize : 1}" ) );
            }
        }
    }

}
