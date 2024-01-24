package com.sequoiadb.recyclebin;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

import java.util.ArrayList;

/**
 * @Description seqDB-29804:回收站接口参数校验
 * @Author liuli
 * @Date 2023.01.06
 * @UpdateAuthor liuli
 * @UpdateDate 2023.01.06
 * @version 1.10
 */
public class RecycleBin29804 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_29804";
    private String clName = "cl_29804";
    private String clNameNew = "cl_new_29804";
    private CollectionSpace dbcs;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        dbcs = sdb.createCollectionSpace( csName );
        dbcs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        DBRecycleBin recycleBin = sdb.getRecycleBin();
        // 使用alter()修改回收站属性，参数指定为null
        try {
            recycleBin.alter( null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // 使用setAttributes()修改回收站属性，参数指定为null
        try {
            recycleBin.setAttributes( null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // 使用dropAll()删除所有回收站项目，参数指定为null
        recycleBin.dropAll( null );

        // 删除cl
        dbcs.dropCollection( clName );
        // 使用getCount()获取回收站项目数量，matcher指定为null
        long count = recycleBin.getCount( null );
        long expCount = 1;
        Assert.assertEquals( expCount, count );

        // 使用returnItem()恢复回收站项目，指定options为null
        BasicBSONObject query = new BasicBSONObject();
        query.put( "OriginName", csName + "." + clName );
        query.put( "OpType", "Drop" );
        String recycleName = getRecycleName( sdb, query ).get( 0 );
        recycleBin.returnItem( recycleName, null );
        Assert.assertTrue( dbcs.isCollectionExist( clName ) );

        // 使用returnItemToName()重命名恢复回收站项目，指定options为null
        dbcs.dropCollection( clName );
        recycleName = getRecycleName( sdb, query ).get( 0 );
        recycleBin.returnItemToName( recycleName, csName + "." + clNameNew,
                null );
        Assert.assertFalse( dbcs.isCollectionExist( clName ) );
        Assert.assertTrue( dbcs.isCollectionExist( clNameNew ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public static ArrayList< String > getRecycleName( Sequoiadb db,
            BasicBSONObject query ) {
        ArrayList< String > recycleNames = new ArrayList<>();
        DBCursor rc = db.getRecycleBin().list( query,
                new BasicBSONObject( "RecycleName", 1 ),
                new BasicBSONObject( "RecycleName", 1 ) );
        while ( rc.hasNext() ) {
            String recycleName = ( String ) rc.getNext().get( "RecycleName" );
            recycleNames.add( recycleName );
        }
        rc.close();
        return recycleNames;
    }

}
