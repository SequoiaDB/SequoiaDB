package com.sequoiadb.subcl;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
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
 * FileName: CreateMainCl26.java test content: 创建子表时指定range分区时的ShardingKey与主表相同
 * testlink case: seqDB-26
 * 
 * @author zengxianquan
 * @date 2016年12月9日
 * @version 1.00
 */
public class CreateMainCl26 extends SdbTestBase {

    private Sequoiadb db = null;
    private String csName = null;
    private String mainclName = "maincl_26";
    private String subclName = "subcl_26";
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.fail(
                    "failed to drop maincl " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void testCreateMainRangSubclBySameShardingKey() {
        csName = SdbTestBase.csName;
        try {
            cs = db.getCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collectionSpace " + e.getMessage() );
        }
        createMaincl();
        attachSubcl();
        checkCRUD();
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );
        try {
            cs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.fail( "Failed to create maincl " + "ErrorMsg:\n"
                    + e.getMessage() );
        }
    }

    public void attachSubcl() {
        BSONObject subclOptions = new BasicBSONObject();
        BSONObject subOpt = new BasicBSONObject();
        subOpt.put( "time", 1 );
        subclOptions.put( "ShardingType", "range" );
        subclOptions.put( "ShardingKey", subOpt );
        try {
            cs.createCollection( subclName, subclOptions );
        } catch ( BaseException e ) {
            Assert.fail( "Failed to create subcl  " + "ErrorMsg:\n"
                    + e.getMessage() );
        }
        BSONObject attachOpt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        BSONObject upBound = new BasicBSONObject();
        lowBound.put( "time", 0 );
        upBound.put( "time", 100 );
        attachOpt.put( "LowBound", lowBound );
        attachOpt.put( "UpBound", upBound );
        try {
            cs.getCollection( mainclName )
                    .attachCollection( csName + "." + subclName, attachOpt );
        } catch ( BaseException e ) {
            Assert.fail( "failed to attach " + "ErrorMsg:\n" + e.getMessage() );
        }
    }

    public void checkCRUD() {
        DBCollection maincl = null;
        try {
            maincl = cs.getCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collection" );
        }
        try {
            maincl.insert( "{time:10,no:10,test:'test'}" );
            DBCursor res = maincl.query();
            String data1 = "{ \"time\" : 10 , \"no\" : 10 , \"test\" : \"test\" }";
            while ( res.hasNext() ) {
                BSONObject dataRes = res.getNext();
                dataRes.removeField( "_id" );
                if ( !( dataRes.toString().equals( data1 ) ) ) {
                    Assert.fail( "failed to query data " );
                }
            }

            maincl.update( "{time:10}", "{$set:{'test':'update'}}", null );
            String data2 = "{ \"time\" : 10 , \"no\" : 10 , \"test\" : \"update\"}";
            DBCursor updateRes = maincl.query();
            while ( res.hasNext() ) {
                BSONObject dataRes = updateRes.getNext();
                dataRes.removeField( "_id" );
                if ( !( dataRes.toString().equals( data2 ) ) ) {
                    Assert.fail( "failed to query data " );
                }
            }

            maincl.delete( "{time:10}" );
            Assert.assertEquals( maincl.getCount(), 0,
                    "failed to delete data" );

        } catch ( BaseException e ) {
            Assert.fail( "failed to CRUD :" + e.getMessage() );
        }
    }

}
