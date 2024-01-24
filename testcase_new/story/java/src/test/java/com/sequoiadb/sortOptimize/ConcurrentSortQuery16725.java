package com.sequoiadb.sortOptimize;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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

/**
 * @Description seqDB-16725: 并发排序
 * @author liuxiaoxuan
 * @date 2018/12/4
 */
public class ConcurrentSortQuery16725 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs1 = null;
    private CollectionSpace cs2 = null;
    private DBCollection cl1;
    private DBCollection cl2;
    private String csName1 = "java_cs_1";
    private String clName1 = "sort_oom_16725_1";
    private String csName2 = "java_cs_2";
    private String clName2 = "sort_oom_16725_2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        try {
            sdb.dropCollectionSpace( csName1 );
        } catch ( BaseException e ) {
            Assert.assertEquals( -34, e.getErrorCode() );
        }
        try {
            sdb.dropCollectionSpace( csName2 );
        } catch ( BaseException e ) {
            Assert.assertEquals( -34, e.getErrorCode() );
        }

        cs1 = sdb.createCollectionSpace( csName1 );
        cl1 = cs1.createCollection( clName1 );
        cs2 = sdb.createCollectionSpace( csName2 );
        cl2 = cs2.createCollection( clName2 );
    }

    @Test
    public void test() {
        insertData( cl1, 400000 ); // insert 40w, larger than 256M
        insertData( cl2, 400000 ); // insert 40w, larger than 256M

        int threadNums = 5;
        QueryThread queryThread = new QueryThread();
        queryThread.start( threadNums );

        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        sdb.dropCollectionSpace( csName1 );
        sdb.dropCollectionSpace( csName2 );
    }

    public void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                BSONObject record = ( BSONObject ) JSON.parse(
                        "{a:'" + Utils.getRandomString( 256 ) + "', b: '"
                                + Utils.getRandomString( 512 ) + "', c: '"
                                + Utils.getRandomString( 255 ) + "'}" );
                records.add( record );
            }
            cl.bulkInsert( records, 0 );
            records.clear();
        }
    }

    private class QueryThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db.getCollectionSpace( csName1 )
                    .getCollection( clName1 );
            DBCollection cl2 = db.getCollectionSpace( csName2 )
                    .getCollection( clName2 );

            // check records
            BSONObject sortObj = new BasicBSONObject();
            sortObj.put( "a", 1 );
            sortObj.put( "b", 1 );
            sortObj.put( "c", 1 );

            Assert.assertTrue( Utils.checkSortResult( cl1, sortObj,
                    Thread.currentThread().getName() ) );
            Assert.assertTrue( Utils.checkSortResult( cl2, sortObj,
                    Thread.currentThread().getName() ) );
            db.close();
        }
    }
}
