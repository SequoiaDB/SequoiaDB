package com.sequoiadb.autoIncrement;

/**
 * @FileName:seqDB-15966： 不指定自增字段插入与truncate并发
 * 预置条件:集合已存在，自增字段已存在，且AcquireSize为1
 * 测试步骤： 不指定自增字段插入的同时，执行truncate操作
 * 预期结果：记录插入成功，记录的自增字段值正确，truncate执行成功，truncate执行之后，
 *       重新又startValue生成自增字段值
 * @Author zhaoxiaoni
 * @Date 2018-11-16
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

public class Insert15966 extends SdbTestBase {
    private Sequoiadb commSdb = null;
    private String clName = "cl_15966";
    private int threadInsertNum = 2000;
    private int increment = 1;
    private long acquireSize = 1;

    @BeforeClass
    public void setUp() {
        commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        DBCollection cl = commSdb.getCollectionSpace( csName )
                .createCollection( clName, ( BSONObject ) JSON.parse(
                        "{AutoIncrement:{Field:'id', Increment:" + increment
                                + ",AcquireSize:" + acquireSize + "}}" ) );
    }

    @Test
    public void test() {
        Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        InsertThread insertThread = new InsertThread();
        insertThread.start();

        try {
            Thread.sleep( 500 );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        TruncateThread truncateThread = new TruncateThread();
        truncateThread.start();

        if ( !( insertThread.isSuccess() && truncateThread.isSuccess() ) ) {
            Assert.fail(
                    insertThread.getErrorMsg() + truncateThread.getErrorMsg() );
        }

        for ( int i = 0; i < threadInsertNum; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{b:" + i + "}" );
            cl.insert( obj );
        }

        // check records count
        long count = ( long ) cl.getCount();
        if ( count < 2000 ) {
            Assert.fail( "records count error!" );
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
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class TruncateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" ) ;) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncate();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }
}
