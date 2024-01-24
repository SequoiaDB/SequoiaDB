package com.sequoiadb.sortOptimize;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

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
 * @Description seqDB-16719: 排序内存分配优化
 * @author liuxiaoxuan
 * @date 2018/12/4
 */
public class ConcurrentSortQuery16719 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs = null;
    private DBCollection cl;
    private String clName = "sort_oom_16719";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName );
    }

    @Test
    public void test() {
        insertData( cl, 200000 ); // insert 20w

        int threadNums = 5;
        QueryThread queryThread = new QueryThread();
        queryThread.start( threadNums );

        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
    }

    public void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                BSONObject record = ( BSONObject ) JSON
                        .parse( "{a:'" + Utils.getRandomString( 64 ) + "', b: '"
                                + Utils.getRandomString( 128 ) + "', c: '"
                                + Utils.getRandomString( 64 ) + "'}" );
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
            DBCollection newcl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            // check records
            BSONObject sortObj = new BasicBSONObject();
            sortObj.put( "a", 1 );
            sortObj.put( "b", 1 );
            sortObj.put( "c", 1 );
            Assert.assertTrue( Utils.checkSortResult( newcl, sortObj,
                    Thread.currentThread().getName() ) );

            db.close();
        }
    }
}
