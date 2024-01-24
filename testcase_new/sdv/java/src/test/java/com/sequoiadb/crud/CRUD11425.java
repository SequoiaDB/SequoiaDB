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
 * @Description seqDB-11425:并发插入与删除cl并发
 * @Author laojingtang
 * @Date 2018.01.04
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2021.07.09
 * @Version 1.10
 */
public class CRUD11425 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl_11425";
    private DBCollection cl = null;
    private boolean dropCLTheadStatus = false;
    private boolean insertDataTheadStatus = false;
    private ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        threadInsert insertData = new threadInsert();
        es.addWorker( insertData );
        threadDropCL dropCL = new threadDropCL();
        es.addWorker( dropCL );
        es.run();
        Assert.assertEquals( insertData.getRetCode(), 0 );
        Assert.assertEquals( dropCL.getRetCode(), 0 );
        if ( dropCLTheadStatus == false && insertDataTheadStatus == true ) {
            CRUDUitls.checkRecords( cl, insertRecord, "" );
        } else if ( dropCLTheadStatus ) {
            if ( cs.isCollectionExist( clName ) ) {
                throw new Exception( "cl drop faild" );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
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
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_DELETING
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class threadDropCL extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs1.dropCollection( clName );
                dropCLTheadStatus = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
