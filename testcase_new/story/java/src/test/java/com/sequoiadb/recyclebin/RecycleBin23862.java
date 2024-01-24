package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;

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
 * @Description seqDB-23862：并发对不同CL执行dropCL，完成后并发恢复多个项目
 * @Author liuli
 * @Date 2021.06.30
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.14
 * @version 1.00
 */
@Test(groups = "recycleBin")
public class RecycleBin23862 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23862";
    private String clName = "cl_23862_";
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
        // 创建CL写入1000条数据，然后删除CL
        for ( int i = 0; i < 10; i++ ) {
            DBCollection dbcl = dbcs.createCollection( clName + i, option );
            insertRecords = insertData( dbcl );
            dbcs.dropCollection( clName + i );
        }
    }

    @Test
    public void test() throws Exception {
        BasicBSONObject option = new BasicBSONObject();
        option.put( "OriginName",
                new BasicBSONObject( "$regex", csName + "." + clName + "*" ) );
        option.put( "OpType", "Drop" );
        List< String > recycleNames = RecycleBinUtils.getRecycleName( sdb,
                option );
        Assert.assertEquals( recycleNames.size(), 10 );

        ThreadExecutor es = new ThreadExecutor( 1200000 );
        for ( String recycleName : recycleNames ) {
            es.addWorker( new ReturnItem( recycleName ) );
        }
        es.run();

        for ( int i = 0; i < recycleNames.size(); i++ ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames.get( i ) );
            DBCollection dbcl1 = sdb.getCollectionSpace( csName )
                    .getCollection( clName + i );
            RecycleBinUtils.checkRecords( dbcl1, insertRecords, "{ a:1 }" );
        }
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

    private class ReturnItem {
        private String recycleName;

        private ReturnItem( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void returnItem() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
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
