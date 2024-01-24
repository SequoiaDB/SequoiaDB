package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23869:并发强制恢复相同回收站项目
 * @Author liuli
 * @Date 2021.07.01
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.00
 */
@Test(groups = "recycleBin")
public class RecycleBin23869 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23869";
    private String clName = "cl_23869";
    private String clNameNew = "cl_23869_new";
    private String originName = csName + "." + clName;
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName );

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        DBCollection dbcl = dbcs.createCollection( clName, option );
        // 写入1000条数据
        int recordNum = 1000;
        insertRecords = RecycleBinUtils.insertData( dbcl, recordNum );
        // truncate 后 renameCL
        dbcl.truncate();
        dbcs.renameCollection( clName, clNameNew );
    }

    @Test
    public void test() throws Exception {
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, originName, "Truncate" ).get( 0 );
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < 10; i++ ) {
            es.addWorker( new ReturnItemEnforced( recycleName ) );
        }
        es.run();

        RecycleBinUtils.checkRecycleItem( sdb, recycleName );
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            sdb.dropCollectionSpace( csName );
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class ReturnItemEnforced {
        private String recycleName;

        private ReturnItemEnforced( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void returnItemEnforced() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject option = new BasicBSONObject();
                option.put( "Enforced", true );
                db.getRecycleBin().returnItem( recycleName, option );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
