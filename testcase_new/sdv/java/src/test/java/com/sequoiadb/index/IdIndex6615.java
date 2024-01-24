package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 用例要求： 1、向cl中插入大量数据（如1千万条记录）
 * 2、使用排序方式创建ID索引，执行创建索引命令createIdIndex({SortBufferSize:256})，默认使用排序缓存
 * 3、创建索引过程中删除cl数据 4、查看创建索引结果和数据删除结果
 *
 * @author huangwenhua
 * @version 1.00
 * @Date 2016.12.14
 */

public class IdIndex6615 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "c6615";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        createCL();
        insertData();
    }

    class Delete extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            @SuppressWarnings("resource")
            Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl2 = sdb2.getCollectionSpace( csName )
                    .getCollection( clName );
            try {
                cl2.delete( "{age:10}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -279 ) {
                    throw e;
                }
            } finally {
                if ( sdb2 != null ) {
                    sdb2.disconnect();
                }
            }
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    public void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        try {
            String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                    + "ReplSize:0,Compressed:true,AutoIndexId:false}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    public void insertData() {
        try {
            int count = 0;
            List< BSONObject > list = new ArrayList<>( 10000 );
            for ( int i = 0; i < 100000; i++ ) {
                BSONObject bson = new BasicBSONObject();
                bson.put( "age", i );
                bson.put( "name", "Json" );
                list.add( bson );
                count++;
                if ( count % 10000 == 0 ) {
                    cl.insert( list );
                    list.clear();
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( " IdIndex6615 insert error:" + e.getMessage() );
        }
    }

    /**
     * 检查琐索引
     */
    public void checkIndex( DBCollection cl ) {
        DBCursor cursor1 = null;
        try {
            // 通过explain，判断是否走索引
            cursor1 = cl.explain( null, null, null,
                    ( BSONObject ) JSON.parse( "{'':'$id'}" ), 0, -1, 0, null );
            String scanType = null;
            while ( cursor1.hasNext() ) {
                BSONObject record = cursor1.getNext();
                if ( record.get( "Name" )
                        .equals( SdbTestBase.csName + "." + clName ) ) {
                    scanType = ( String ) record.get( "ScanType" );
                }
            }
            Assert.assertEquals( scanType, "ixscan" );
        } catch ( BaseException e ) {
            Assert.fail( " IdIndex6614 check error:" + e.getMessage() );
        } finally {
            cursor1.close();
        }
    }

}
