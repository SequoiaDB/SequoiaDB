package com.sequoiadb.crud;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-11426:并发插入与删除cs并发
 * @Author wuyan
 * @Date 2017.11.09
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2021.07.09
 * @version 1.00
 */

public class CRUD11426 extends SdbTestBase {
    private final String clName = "cl_11426";
    private final String csName = "testcs_11426";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private boolean dropCSTheadStatus = false;
    private boolean insertDataTheadStatus = false;
    private ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {

        ThreadExecutor es = new ThreadExecutor();
        threadInsert insertData1 = new threadInsert();
        es.addWorker( insertData1 );
        threadDropCS dropCS = new threadDropCS();
        es.addWorker( dropCS );
        es.run();
        Assert.assertEquals( insertData1.getRetCode(), 0 );
        Assert.assertEquals( dropCS.getRetCode(), 0 );
        if ( dropCSTheadStatus == false && insertDataTheadStatus == true ) {
            CRUDUitls.checkRecords( cl, insertRecord, "" );
        } else if ( dropCSTheadStatus ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                throw new Exception( "cs drop faild" );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class threadDropCS extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                db.dropCollectionSpace( csName );
                dropCSTheadStatus = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class threadInsert extends ResultStore {
        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db.getCollectionSpace( csName );
                DBCollection cl = cs1.getCollection( clName );
                for ( int i = 0; i < 10000; i++ ) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put( "a", i );
                    obj.put( "no", i );
                    insertRecord.add( obj );
                    cl.insert( obj );
                }
                insertDataTheadStatus = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_DELETING
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
