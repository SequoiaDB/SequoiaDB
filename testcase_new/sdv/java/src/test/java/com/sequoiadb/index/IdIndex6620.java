package com.sequoiadb.index;

import org.bson.BSONObject;
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

/**
 * 用例要求： 1、指定已存在id索引的cl，删除集合中的 $id 索引(dropIdIndex()) 2、查看索引删除结果
 * 3、对该cl分别执行插入、更新、删除数据操作 4、查看返回的结果是否正确
 * 
 * @author huangwenhua
 * @Date 2016.12.15
 * @version 1.00
 */
public class IdIndex6620 extends SdbTestBase {
    private Sequoiadb sdb;

    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "c6620";

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( " IdIndex6614 setUp error:" + e.getMessage() );
        }
        createCL();
        dropIdIndex();
    }

    @Test
    public void checkIndex() {
        try {
            cl.insert( "{a:1,age:2}" );
        } catch ( BaseException e ) {
            Assert.fail( "insert error:" + e.getMessage() );
        }
        try {
            cl.update( null, "{$set:{age:4}}", null );

        } catch ( BaseException e ) {
            Assert.assertEquals( -279, e.getErrorCode(), e.getMessage() );
        }
        try {
            cl.delete( "{a:1}" );
        } catch ( BaseException e ) {
            Assert.assertEquals( -279, e.getErrorCode(), e.getMessage() );
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
            if ( !this.sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                this.sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        try {
            String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                    + "ReplSize:0,Compressed:true}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            this.cl = this.cs.createCollection( this.clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    public void dropIdIndex() {
        DBCursor cursor1 = null;
        try {
            cl.dropIdIndex();
            // 通过explain，是否走索引,判断索引是否删除成功
            cursor1 = cl.explain( null, null, null,
                    ( BSONObject ) JSON.parse( "{'':'$id'}" ), 0, -1, 0, null );
            String scanType = null;
            while ( cursor1.hasNext() ) {
                BSONObject record = cursor1.getNext();
                if ( record.get( "Name" )
                        .equals( SdbTestBase.csName + "." + this.clName ) ) {
                    scanType = ( String ) record.get( "ScanType" );
                }
            }
            Assert.assertEquals( scanType, "tbscan" );

        } catch ( BaseException e ) {
            Assert.fail( "dropIdIndex error:" + e.getMessage() );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
        }
    }
}
