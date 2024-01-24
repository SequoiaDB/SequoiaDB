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
 * @Description seqDB-23870:并发强制恢复不同回收站项目
 * @Author liuli
 * @Date 2021.07.01
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin23870 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23870_";
    private String csNameNew = "cs_23870_new_";
    private String clName = "cl_23870_";
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

        for ( int i = 0; i < 10; i++ ) {
            if ( sdb.isCollectionSpaceExist( csNameNew + i ) ) {
                sdb.dropCollectionSpace( csNameNew + i );
            }
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );

        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        // 创建CL写入1000条数据，然后删除CL
        for ( int i = 0; i < 10; i++ ) {
            CollectionSpace dbcs = sdb.createCollectionSpace( csName + i );
            DBCollection dbcl = dbcs.createCollection( clName + i, option );
            insertRecords = insertData( dbcl );
            dbcs.dropCollection( clName + i );
            sdb.renameCollectionSpace( csName + i, csNameNew + i );
        }
    }

    @Test
    public void test() throws Exception {
        BasicBSONObject query = new BasicBSONObject();
        query.put( "OriginName", new BasicBSONObject( "$regex",
                csName + "[0-9]." + clName + "[0-9]" ) );
        List< String > recycleNames = RecycleBinUtils.getRecycleName( sdb,
                query );
        Assert.assertEquals( recycleNames.size(), 10 );
        ThreadExecutor es = new ThreadExecutor( 300000 );
        for ( String recycleName : recycleNames ) {
            es.addWorker( new ReturnItemEnforced( recycleName ) );
        }
        es.run();

        for ( int i = 0; i < recycleNames.size(); i++ ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames.get( i ) );
            DBCollection dbcl = sdb.getCollectionSpace( csNameNew + i )
                    .getCollection( clName + i );
            RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
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
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
            for ( int i = 0; i < 10; i++ ) {
                if ( sdb.isCollectionSpaceExist( csNameNew + i ) ) {
                    sdb.dropCollectionSpace( csNameNew + i );
                }
            }
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );
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
