package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 用例要求： 1、向cl中插入大量数据（如1千万条记录）
 * 2、创建离线方式ID索引，执行创建索引命令createIdIndex({SortBufferSize:16}) 3、创建索引过程中向该CL更新数据
 * 4、查看创建索引结果和数据更新结果
 *
 * @author huangwenhua
 * @version 1.00
 * @Date 2016.12.14
 */
public class IdIndex6614 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "c6614";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        createCL();
        insertData();
    }

    @Test
    public void upsertData() {
        UpdateTask updateTask = new UpdateTask();
        CreateIndex indexThread = new CreateIndex();
        updateTask.start();
        indexThread.start();

        updateTask.join();
        indexThread.join();
        Assert.assertTrue( indexThread.isSuccess(), indexThread.getErrorMsg() );
        Assert.assertTrue( updateTask.isSuccess(), updateTask.getErrorMsg() );
    }

    class UpdateTask extends SdbThreadBase {

        @Override
        @SuppressWarnings("deprecation")
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( IdIndex6614.this.clName );
                cl.update( null, "{$set:{name:\"kkkk\"}}", null );
                checkUpdated( cl );
            } catch ( BaseException e ) {
                // SDB_RTN_AUTOINDEXID_IS_FALSE(-279): can not update/delete
                // records when $id index does not exist
                if ( e.getErrorCode() == -279 ) {
                    checkNoUpdate( cl );
                } else {
                    throw e;
                }
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }

        private List< BSONObject > getActualRecord( DBCollection cl ) {
            DBCursor cur = cl.query();
            List< BSONObject > list = new ArrayList<>( 10000 );
            while ( cur.hasNext() ) {
                list.add( cur.getNext() );
            }
            cur.close();
            return list;
        }

        private void checkUpdated( DBCollection cl ) {
            List< BSONObject > list = getActualRecord( cl );
            for ( BSONObject object : list ) {
                Assert.assertEquals( object.get( "name" ), "kkkk",
                        object.toString() );
            }
        }

        private void checkNoUpdate( DBCollection cl ) {
            List< BSONObject > list = getActualRecord( cl );
            for ( BSONObject object : list ) {
                Assert.assertNotEquals( object.get( "name" ), "kkkk",
                        object.toString() );
            }
        }
    }

    /**
     * 并发创建索引
     */
    class CreateIndex extends SdbThreadBase {
        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws BaseException {

            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl1 = db1.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject indexObj = ( BSONObject ) JSON
                        .parse( "{SortBufferSize:16}" );
                cl1.createIdIndex( indexObj );
                checkIdIndex( cl1 );
            } finally {
                if ( db1 != null ) {
                    db1.disconnect();
                }
            }
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    void createCL() {
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true,AutoIndexId:false}";
        BSONObject options = ( BSONObject ) JSON.parse( clOptions );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName, options );
    }

    void insertData() {
        List< BSONObject > list = new ArrayList<>( 10000 );
        for ( int i = 0; i < 10000; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "age", i );
            bson.put( "name", "Json" );
            list.add( bson );
        }
        cl.insert( list );
    }

    /**
     * 检查索引
     */
    void checkIdIndex( DBCollection cl ) {
        // 通过explain，判断是否走索引
        DBCursor cursor1 = cl.explain( null, null, null,
                ( BSONObject ) JSON.parse( "{'':'$id'}" ), 0, -1, 0, null );
        String scanType = null;
        String indexName = null;
        while ( cursor1.hasNext() ) {
            BSONObject record = cursor1.getNext();
            if ( record.get( "Name" )
                    .equals( SdbTestBase.csName + "." + clName ) ) {
                scanType = ( String ) record.get( "ScanType" );
                indexName = ( String ) record.get( "IndexName" );
            }
        }
        Assert.assertEquals( scanType, "ixscan" );
        Assert.assertEquals( indexName, "$id" );
        cursor1.close();
    }
}
