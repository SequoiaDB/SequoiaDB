package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: CRUDMainCL832.java test content: 多子表大数据量时对主表做增删改操作_SD.subCL.01.019
 * testlink case: seqDB-832
 * 
 * @author zengxianquan
 * @date 2016年12月21日
 * @version 1.00
 */

public class CRUDMainCL832 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace maincs = null;
    private String mainclName = "maincl832";
    private AtomicInteger doneCount = new AtomicInteger( 0 );
    private final int THREAD_COUNT = 5;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            maincs = sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( SubCLUtils.getDataGroups( sdb ).size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups" );
        }
        createMaincl();
        createSubcls();
        attachSubcls();
    }

    @AfterClass
    public void tearDown() {
        try {
            maincs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingType", "range" );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        options.put( "ShardingKey", opt );
        try {
            maincs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "createMaincl failed " + e.getMessage() );
        }
    }

    public void createSubcls() {
        try {
            for ( int j = 0; j < 5; j++ ) {
                maincs.createCollection( "subcl832_" + j );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "createSubcl failed " + e.getMessage() );
        }
    }

    public void attachSubcls() {
        DBCollection maincl = null;
        try {
            maincl = maincs.getCollection( mainclName );
            for ( int j = 0; j < 5; j++ ) {
                String jsonStr = "{'LowBound':{'a':" + j * 100
                        + "},UpBound:{'a':" + ( j + 1 ) * 100 + "}}";
                BSONObject options = ( BSONObject ) JSON.parse( jsonStr );
                maincl.attachCollection( SdbTestBase.csName + ".subcl832_" + j,
                        options );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @DataProvider(name = "diffDataProvider", parallel = true)
    public Object[][] createData() {
        return new Object[][] { new Object[] { "a" }, new Object[] { "b" },
                new Object[] { "c" }, new Object[] { "d" },
                new Object[] { "e" }, };
    }

    @Test(dataProvider = "diffDataProvider")
    public void test( String diffData ) {
        Sequoiadb db = null;
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            // 检验插入数据
            insertAndCheckData( cs, maincl, diffData );
            // 检验更新数据
            updateAndCheckData( cs, maincl, diffData );
            // 检验删除数据
            deleteAndCheckData( cs, maincl, diffData );
            // 检验数据的完整性
            if ( doneCount.incrementAndGet() == THREAD_COUNT ) {
                if ( maincl.getCount() != 500 ) {
                    Assert.fail( " The data count is not correct" );
                }
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    public void insertAndCheckData( CollectionSpace cs, DBCollection maincl,
            String diffData ) {
        // 构造插入的数据
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < 500; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "a", i );
            bson.put( "test", diffData );
            insertor.add( bson );
        }
        try {
            // 插入数据
            maincl.insert( insertor );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "test", diffData );
        int dataCount = 500;
        checkData( cs, matcher, diffData, dataCount );
    }

    public void updateAndCheckData( CollectionSpace cs, DBCollection maincl,
            String diffData ) {
        try {
            maincl.update( "{test:'" + diffData + "'}",
                    "{$set:{'test':'" + diffData + "Update'}}", null );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "test", diffData + "Update" );
        String expectStr = diffData + "Update";
        int dataCount = 500;
        checkData( cs, matcher, expectStr, dataCount );
    }

    public void deleteAndCheckData( CollectionSpace cs, DBCollection maincl,
            String diffData ) {
        try {
            BSONObject options = new BasicBSONObject();
            BSONObject opt = new BasicBSONObject();
            opt.put( "$gte", 100 );
            options.put( "a", opt );
            options.put( "test", diffData + "Update" );
            maincl.delete( options );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "test", diffData + "Update" );
        String expectStr = diffData + "Update";
        int dataCount = 100;
        checkData( cs, matcher, expectStr, dataCount );
    }

    public void checkData( CollectionSpace cs, BSONObject matcher,
            String expectStr, int dataCount ) {
        DBCollection maincl = null;
        DBCursor cursor = null;
        try {
            maincl = cs.getCollection( mainclName );
            BSONObject order = new BasicBSONObject();
            order.put( "a", 1 );
            cursor = maincl.query( matcher, null, order, null );
            int j = 0;
            BSONObject res;
            while ( cursor.hasNext() ) {
                res = cursor.getNext();
                res.removeField( "_id" );
                String data = "{ \"a\" : " + j + " , \"test\" : \"" + expectStr
                        + "\" }";
                if ( !( res.toString().equals( data ) ) ) {
                    Assert.fail( "failed to query data " );
                }
                j++;
            }
            // 检验数据的完整性，如果j与预期的数量不一致则出错
            if ( j != dataCount ) {
                Assert.fail( "data count is error" );
            }
        } catch ( BaseException e ) {
            Assert.fail( "failed to query data " + e.getMessage() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }

    }
}