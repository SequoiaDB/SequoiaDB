package com.sequoiadb.transaction.metadata;

/**
 * @FileName:SEQDB-10537 切分过程中执行事务操作
 * @author huangqiaohui
 * @version 1.00
 *
 */
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test
public class Transaction10537 extends SdbTestBase {
    private String clName = "testcaseCL10537";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb sdb = null;

    @BeforeClass()
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > groupsNames = CommLib.getDataGroupNames( sdb );
        if ( groupsNames.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        srcGroupName = groupsNames.get( 0 );
        destGroupName = groupsNames.get( 1 );

        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName, ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                + groupsNames.get( 0 ) + "'}" ) );
        insertData( cl );
    }

    @Test
    public void transaction() {
        Sequoiadb db = null;
        DBCollection cl = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );

            TransOperations transOperations = new TransOperations();
            transOperations.start();

            Split split = new Split();
            split.start();

            Assert.assertTrue( split.isSuccess(), split.getErrorMsg() );
            Assert.assertTrue( transOperations.isSuccess(),
                    transOperations.getErrorMsg() );

            checkDestAndSrcGroup();
            queryUpdatedAndDeletedData( cl );
        } finally {
            db.close();
        }
    }

    @AfterClass()
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.close();
    }

    private void queryUpdatedAndDeletedData( DBCollection cl ) {
        DBCursor cusor = null;
        try {
            List< BSONObject > expectData = new ArrayList< >();
            for ( int i = 40000; i < 60000; i++ ) {
                expectData.add(
                        ( BSONObject ) JSON.parse( "{sk:" + i + ",beta:2}" ) );
            }

            cusor = cl.query( "{sk:{$gte:40000,$lt:60000}}", null, null, null );
            while ( cusor.hasNext() ) {
                BSONObject obj = cusor.getNext();
                obj.removeField( "_id" );
                if ( expectData.contains( obj ) ) {
                    expectData.remove( obj );
                } else {
                    Assert.fail( "should not find this record:" + obj );
                }
            }
            Assert.assertEquals( expectData.size(), 0,
                    "miss some record:" + expectData );
        } finally {
            if ( cusor != null ) {
                cusor.close();
            }
        }
    }

    class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:50000}" ),
                        ( BSONObject ) JSON.parse( "{sk:100000}" ) );
            } finally {
                db.close();
            }
        }
    }

    class TransOperations extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                TransUtils.beginTransaction( db );
                cl.delete( "{sk:{$gte:40000,$lt:60000}}" );
                for ( int i = 40000; i < 60000; i++ ) {
                    cl.insert( "{sk:" + i + ",beta:1}" );
                }
                cl.update( "{sk:{$gte:40000,$lt:60000}}", "{$inc:{beta:1}}",
                        null );
            } finally {
                db.commit();
                db.close();
            }
        }
    }

    private void insertData( DBCollection cl ) {
        List< BSONObject > insertedData = new ArrayList< >();
        for ( int i = 0; i < 100000; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{sk:" + i + ",alpha:" + i + "}" );
            insertedData.add( obj );
        }
        cl.insert( insertedData );
    }

    private void checkDestAndSrcGroup() {
        List< BSONObject > srcExpect = new ArrayList< >();
        for ( int i = 0; i < 40000; i++ ) {
            srcExpect.add( ( BSONObject ) JSON
                    .parse( "{sk:" + i + ",alpha:" + i + "}" ) );
        }
        for ( int i = 40000; i < 50000; i++ ) {
            srcExpect.add(
                    ( BSONObject ) JSON.parse( "{sk:" + i + ",beta:2}" ) );
        }
        checkGroupData( sdb, srcGroupName, srcExpect );

        List< BSONObject > destExpect = new ArrayList< >();
        for ( int i = 50000; i < 60000; i++ ) {
            destExpect.add(
                    ( BSONObject ) JSON.parse( "{sk:" + i + ",beta:2}" ) );
        }
        for ( int i = 60000; i < 100000; i++ ) {
            destExpect.add( ( BSONObject ) JSON
                    .parse( "{sk:" + i + ",alpha:" + i + "}" ) );
        }
        checkGroupData( sdb, destGroupName, destExpect );
    }

    private void checkGroupData( Sequoiadb db, String groupName,
            List< BSONObject > expect ) {
        Sequoiadb dataNode = null;
        DBCollection cl = null;
        DBCursor cursor = null;
        try {
            dataNode = db.getReplicaGroup( groupName ).getMaster().connect();
            cl = dataNode.getCollectionSpace( csName ).getCollection( clName );
            List< BSONObject > actual = new ArrayList< >();
            cursor = cl.query( null, null, "{sk:1}", null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                obj.removeField( "_id" );
                actual.add( obj );
            }
            Assert.assertEquals( expect.equals( actual ), true,
                    "expect:" + expect + "\r\nactual:" + actual );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( dataNode != null ) {
                dataNode.close();
            }
        }
    }

}
