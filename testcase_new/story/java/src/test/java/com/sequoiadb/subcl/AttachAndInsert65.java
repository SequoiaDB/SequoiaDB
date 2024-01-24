package com.sequoiadb.subcl;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: AttachAndInsert65 test content:
 * 不同coord连接，在一个coord节点上attach完子表，马上在另一个coord节点上写入数据 testlink case: seqDB65
 * 
 * @author zengxianquan
 * @date 2016年12月17日
 * @version 1.00
 */
public class AttachAndInsert65 extends SdbTestBase {
    private Sequoiadb sdb1;
    private Sequoiadb sdb2;
    private String mainclName = "maincl65";
    private String subclName = "subcl65";
    private List< String > addressList = null;

    @BeforeClass
    public void setUp() {
        try ( Sequoiadb tmpdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( SubCLUtils.isStandAlone( tmpdb ) ) {
                throw new SkipException( "is standalone skip testcase" );
            }
            addressList = SubCLUtils.getNodeAddress( tmpdb, "SYSCoord" );
            if ( addressList.size() < 2 ) {
                throw new SkipException( "coord quantity is less than 2" );
            }
            sdb1 = new Sequoiadb( addressList.get( 0 ), "", "" );
            sdb2 = new Sequoiadb( addressList.get( 1 ), "", "" );
        }
        createCl( sdb1 );
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = null;
        try ( Sequoiadb tmpdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            cs = tmpdb.getCollectionSpace( SdbTestBase.csName );
            cs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            sdb1.close();
            sdb2.close();
        }
    }

    @Test
    public void test() {
        attachSubcl( sdb1 );
        insert( sdb2 );
    }

    public void createCl( Sequoiadb db ) {
        CollectionSpace cs = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            BSONObject mainObj = ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{age:1},"
                            + "ShardingType:\"range\"}" );
            cs.createCollection( mainclName, mainObj );

            BSONObject subObj = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{age:1},"
                            + "ShardingType:\"hash\",Partition:1024}" );
            cs.createCollection( subclName, subObj );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void attachSubcl( Sequoiadb db ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            BSONObject options = ( BSONObject ) JSON
                    .parse( "{LowBound:{age:50},UpBound:{age:100}}" );
            maincl.attachCollection( SdbTestBase.csName + "." + subclName,
                    options );
        } catch ( BaseException e ) {
            Assert.fail( "attach is faild:" + e.getMessage() );
        }
    }

    public void insert( Sequoiadb db ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            for ( int i = 50; i < 100; i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "name", "tmo" + i );
                obj.put( "age", i );
                maincl.insert( obj );
            }
        } catch ( BaseException e ) {
            Assert.fail( "insert is error:" + e.getMessage() );
        }
        for ( int i = 100; i < 150; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "name", "tmo" + i );
            obj.put( "age", i );
            try {
                maincl.insert( obj );
                Assert.fail( "insert is error" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -135, e.getMessage() );
            }
        }
        for ( int i = 0; i < 50; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "name", "tmo" + i );
            obj.put( "age", i );
            try {
                maincl.insert( obj );
                Assert.fail( "insert is error" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -135, e.getMessage() );
            }
        }
    }
}
