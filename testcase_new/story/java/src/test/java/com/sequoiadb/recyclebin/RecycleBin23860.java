package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23860：并发对不同CS执行dropCS，完成后并发恢复多个项目
 * @Author liuli
 * @Date 2021.06.30
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.14
 * @version 1.00
 */
@Test(groups = "recycleBin")
public class RecycleBin23860 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23860_";
    private String clName = "cl_23860_";
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        for ( int i = 0; i < 10; i++ ) {
            if ( sdb.isCollectionSpaceExist( csName + i ) ) {
                sdb.dropCollectionSpace( csName + i );
            }
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName );

        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        for ( int i = 0; i < 10; i++ ) {
            CollectionSpace dbcs = sdb.createCollectionSpace( csName + i );
            DBCollection dbcl = dbcs.createCollection( clName + i, option );
            // 写入1000条数据
            insertRecords = insertData( dbcl );
        }

        // 删除CS
        for ( int i = 0; i < 10; i++ ) {
            sdb.dropCollectionSpace( csName + i );
        }
    }

    @Test
    public void test() throws Exception {

        List< String > recycleNames = RecycleBinUtils.getRecycleName( sdb,
                new BasicBSONObject( "OriginName",
                        new BasicBSONObject( "$regex", csName + "*" ) ) );
        Assert.assertEquals( recycleNames.size(), 10 );
        // 并发恢复回收站项目
        ThreadExecutor es = new ThreadExecutor( 600000 );
        for ( String recycleName : recycleNames ) {
            es.addWorker( new ReturnItem( recycleName ) );
        }
        es.run();

        // 检测回收站项目被删除并校验数据
        for ( int i = 0; i < recycleNames.size(); i++ ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames.get( i ) );
            DBCollection dbcl1 = sdb.getCollectionSpace( csName + i )
                    .getCollection( clName + i );
            RecycleBinUtils.checkRecords( dbcl1, insertRecords, "{ a:1 }" );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            for ( int i = 0; i < 10; i++ ) {
                if ( sdb.isCollectionSpaceExist( csName + i ) ) {
                    sdb.dropCollectionSpace( csName + i );
                }
            }
        }
        RecycleBinUtils.cleanRecycleBin( sdb, "cs_23860" );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class ReturnItem {
        private String recycleName;

        private ReturnItem( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void returnItem() {
            String originName = "";
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCursor cursor = db.getRecycleBin().list(
                        new BasicBSONObject( "RecycleName", recycleName ), null,
                        null );
                originName = ( String ) cursor.getNext().get( "OriginName" );
                cursor.close();
                db.getRecycleBin().returnItem( recycleName, null );
            }
        }
    }

    private List< BSONObject > insertData( DBCollection cl ) {
        List< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < 1000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "_id", i );
            obj.put( "a", i );
            obj.put( "num", i );
            insertRecords.add( obj );
        }
        cl.bulkInsert( insertRecords );
        return insertRecords;
    }
}
