package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
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
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17147:事务中执行ddl操作失败，事务不会回滚
 * @date 2019-1-24
 * @author yinzhen
 *
 */
public class Transaction17147 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db2 = null;
    private String csName = "cs_17147";
    private String clName = "cl_17147";
    private CollectionSpace cs;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( record );
        record = ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" );
        cl.insert( record );
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        db2.commit();
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
    }

    @Test
    public void test() {
        // 开启事务执行增删改操作
        TransUtils.beginTransaction( sdb );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:3, a:3, b:3}" );
        cl.insert( record );
        expList.add( record );
        cl.delete( "{a:1}", "{'':null}" );
        cl.update( "{a:2}", "{$set:{a:4}}", "{'':null}" );
        record = ( BSONObject ) JSON.parse( "{_id:2, a:4, b:2}" );
        expList.add( record );

        // 执行DDL操作构造失败的场景
        try {
            cl.createIndex( "a", "{a:1}", false, false );
            cl.createIndex( "a", "{b:1}", false, false );
            throw new BaseException( -999, "CREATEINDEX ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -46 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            cl.dropIndex( "" );
            throw new BaseException( -999, "DROPINDEX ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            sdb.renameCollectionSpace( "cs117147", "cs217147" );
            throw new BaseException( -999, "RENAMECS ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -34 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            cs.renameCollection( "cl1", "cl2" );
            throw new BaseException( -999, "RENAMECL ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -23 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            sdb.dropCollectionSpace( "cs17147" );
            throw new BaseException( -999, "DROPCS ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -34 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            cs.dropCollection( "cl" );
            throw new BaseException( -999, "DROPCL ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -23 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            cs.alterCollectionSpace(
                    ( BSONObject ) JSON.parse( "{PageSize:4096}" ) );
            throw new BaseException( -999, "ALTERCS ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -275 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            cl.alterCollection(
                    ( BSONObject ) JSON.parse( "{AutoSplit:true}" ) );
            throw new BaseException( -999, "ALTERCL ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -245 ) {
                Assert.fail( e.getMessage() );
            }
        }
        ObjectId oId = new ObjectId();
        try {
            cl.removeLob( oId );
            throw new BaseException( -999, "REMOVELOB ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -4 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            cl.openLob( oId );
            throw new BaseException( -999, "CREATELOB ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -4 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            cl.truncateLob( oId, 12 );
            throw new BaseException( -999, "TRUNCATELOB ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -4 ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            TransUtils.beginTransaction( db2 );
            cl2.truncate();
            throw new BaseException( -999, "TRUNCATE ERROR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -190 ) {
                Assert.fail( e.getMessage() );
            }
        } finally {
            db2.commit();
        }

        // 事务提交
        sdb.commit();

        // 读记录走表扫描
        DBCursor recordsCursor = cl.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 读记录走索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, "{a:1}",
                "{'':'textIndex17147'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );
        recordsCursor.close();
    }
}
