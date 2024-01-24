package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
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

/**
 * @testcase seqDB-20514:事务内不允许切分
 * @date 2020-1-28
 * @author Lena
 * 
 */
@Test
public class Transaction20514 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String clName = "cl20514";
    private String srcGroup;
    private String desGroup;
    private DBCollection cl;
    private CollectionSpace cs;

    @BeforeClass
    public void setUp() {

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone!" );
        }

        List< String > groupsNames = CommLib.getDataGroupNames( sdb );
        if ( groupsNames.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }

        srcGroup = groupsNames.get( 0 );
        desGroup = groupsNames.get( 1 );

        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );

        insertData( cl );
    }

    @Test
    public void test() {

        try {
            TransUtils.beginTransaction( sdb );
            cl.split( srcGroup, desGroup, 50 );
            Assert.fail( "Split should not success!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -315, e.getMessage() );
        } finally {
            sdb.rollback();
        }
        checkSplit( sdb );

    }

    private void insertData( DBCollection cl ) {
        List< BSONObject > insertedData = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{sk:" + i + ",alpha:" + i + "}" );
            insertedData.add( obj );
        }
        cl.insert( insertedData );
    }

    private void checkSplit( Sequoiadb db ) {

        BasicBSONList bsonLists = null;
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                ( BSONObject ) JSON
                        .parse( "{Name:'" + csName + "." + clName + "'}" ),
                null, null );

        while ( cur.hasNext() ) {
            bsonLists = ( BasicBSONList ) cur.getNext().get( "CataInfo" );
        }
        cur.close();
        Assert.assertEquals( bsonLists.size(), 1 );
        Assert.assertEquals( cl.getCount(), 100 );

    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        cs.dropCollection( clName );

        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

}
