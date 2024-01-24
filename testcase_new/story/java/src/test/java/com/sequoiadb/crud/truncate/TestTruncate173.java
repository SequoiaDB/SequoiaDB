package com.sequoiadb.crud.truncate;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-173:insert与truncate的并发 插入数据，一条线程执行insert，另一条线程执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate173 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl173";
    private BSONObject record = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        DBCollection cl = TruncateUtils.createCL( sdb, csName, clName );
        // doing insert
        TruncateUtils.insertData( cl );
        // prepare data for insert(below)
        record = new BasicBSONObject();
        record.put( "key", "123456789" );

    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        InsertThread insertThread = new InsertThread();

        insertThread.start();
        truncateThread.start();

        if ( !( truncateThread.isSuccess() && insertThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + insertThread.getErrorMsg() );
        }
    }

    private class TruncateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            DBCollection cl = null;
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing truncate
                cl.truncate();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -147
                        && e.getErrorCode() != -190 ) {
                    Assert.fail( " truncate fail! " + e.getErrorCode() );
                }
            }
        }
    }

    private class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            DBCollection cl = null;
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing insert
                cl.insert( record );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -147
                        && e.getErrorCode() != -190 ) {
                    Assert.fail( " insert fail! " + e.getErrorCode() );
                }
            }
        }
    }
}
