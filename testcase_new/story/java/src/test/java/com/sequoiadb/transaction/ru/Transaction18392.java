package com.sequoiadb.transaction.ru;

/**
 * @FileName: seqDB-18392:开启事务的过程中执行切分
 * @author zhaoxiaoni
 *
 */
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "ru")
public class Transaction18392 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_18392";
    private String srcGroup = null;
    private String desGroup = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone!" );
        }
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups" );
        }
        srcGroup = groupNames.get( 0 );
        desGroup = groupNames.get( 1 );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName, ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1}, ShardingType:'range', Group:'"
                                + groupNames.get( 0 ) + "'}" ) );
        insertData( cl );
    }

    // SEQUOIADBMAINSTREAM-5432
    @Test(enabled = false)
    public void Test() {
        Sequoiadb db = null;
        DBCollection cl = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            TransUtils.beginTransaction( db );
            insertData( cl );
            cl.split( srcGroup, desGroup, 50 );
            Assert.fail( "Split should not success!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -315, e.getMessage() );
        } finally {
            db.rollback();
        }
        checkSplit( cl );
    }

    @AfterClass
    public void afterClass() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.close();
    }

    private void insertData( DBCollection cl ) {
        List< BSONObject > insertedData = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100000; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{sk:" + i + ",alpha:" + i + "}" );
            insertedData.add( obj );
        }
        cl.insert( insertedData );
    }

    private void checkSplit( DBCollection cl ) {
        List< String > groupNames = new ArrayList<>();
        DBCursor cur = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                ( BSONObject ) JSON
                        .parse( "{Name:'" + csName + "." + clName + "'}" ),
                null, null );
        while ( cur.hasNext() ) {
            BasicBSONList bsonLists = ( BasicBSONList ) cur.getNext()
                    .get( "CataInfo" );
            for ( int i = 0; i < bsonLists.size(); i++ ) {
                BasicBSONObject obj = ( BasicBSONObject ) bsonLists.get( i );
                groupNames.add( obj.getString( "GroupName" ) );
            }
        }
        Set< String > set = new HashSet< String >( groupNames );
        groupNames.clear();
        groupNames.addAll( set );
        Assert.assertEquals( groupNames.size(), 1 );
        Assert.assertEquals( cl.getCount(), 100000 );
    }
}
