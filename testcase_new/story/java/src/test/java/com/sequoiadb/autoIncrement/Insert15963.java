package com.sequoiadb.autoIncrement;

import java.util.ArrayList;

/**
 * @FileName:seqDB-15963： 不指定自增字段插入与修改自增字段属性并发 
 * 预置条件:集合已存在，自增字段已存在，且AcquireSize为1
 * 测试步骤： 不指定自增字段插入与修改自增字段属性并发 
 * 预期结果：记录插入成功，自增字段创建成功,修改属性前，记录按照修改前的属性生成自增字段值，自增字段创建后，
 * coord会清空sequence的缓存，重新获取一次自增字段值，按照修改后的属性生成自增字段值，且之后的值唯一且递增，值正确
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

public class Insert15963 extends SdbTestBase {

    private Sequoiadb commSdb = null;
    private String clName = "cl_15963";
    private int threadInsertNum = 10000;
    private long startValue = 1;
    private long acquireSize = 1000;
    private int befIncrement = 3;
    private int afIncrement = 4;

    @BeforeClass
    public void setUp() {
        commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        DBCollection cl = commSdb.getCollectionSpace( csName ).createCollection(
                clName,
                ( BSONObject ) JSON
                        .parse( "{AutoIncrement:{Field:'id', Increment:"
                                + befIncrement + "}}" ) );
    }

    @Test
    public void test() {
        Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        InsertThread insertThread = new InsertThread();
        insertThread.start();

        AlterThread alterThread = new AlterThread();
        alterThread.start();

        if ( !( insertThread.isSuccess() && alterThread.isSuccess() ) ) {
            Assert.fail(
                    insertThread.getErrorMsg() + alterThread.getErrorMsg() );
        }

        checkResult( sdb, threadInsertNum );
    }

    public void checkResult( Sequoiadb db, int expectNum ) {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );

        // 校验记录数
        int count = ( int ) cl.getCount();
        if ( count != expectNum && count != expectNum + 1 ) {
            Assert.fail( "expect:" + expectNum + "or " + expectNum + 1
                    + ",but actual:" + count );
        }

        // 获取自增字段
        DBCursor cursorS = db.getSnapshot( 8,
                ( BSONObject ) JSON
                        .parse( "{Name:'" + csName + "." + clName + "'}" ),
                null, null );
        ArrayList< String > arrList = new ArrayList< String >();
        while ( cursorS.hasNext() ) {
            ArrayList< BSONObject > record = ( ArrayList< BSONObject > ) cursorS
                    .getNext().get( "AutoIncrement" );
            for ( int i = 0; i < record.size(); i++ ) {
                BSONObject autoIncrement = ( BSONObject ) record.get( i );
                arrList.add( ( String ) autoIncrement.get( "Field" ) );
            }
        }

        // 在自增字段上创建唯一索引
        for ( int i = 0; i < arrList.size(); i++ ) {
            cl.createIndex( "id" + i, "{" + arrList.get( i ) + ":1}", true,
                    false );
        }

        // 比较记录是否包含自增字段
        DBCursor cursorR = cl.query( "{'mustCheckAutoIncrement':{$exists:1}}",
                null, null, null );
        while ( cursorR.hasNext() ) {
            BSONObject record = cursorR.getNext();
            for ( int i = 0; i < arrList.size(); i++ ) {
                boolean hasAutoIncrementField = record
                        .containsField( arrList.get( i ) );
                Assert.assertTrue( hasAutoIncrementField,
                        record + arrList.get( i ) );
            }
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

    private class AlterThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" ) ;) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.setAttributes( ( BSONObject ) JSON
                        .parse( "{AutoIncrement:{Field:'id',Increment:"
                                + afIncrement + "}}" ) );
            }
        }
    }

}
