package com.sequoiadb.sdb;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Parameters;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSnapshot10371 seqDB-10371 test interface: getSnapshot (int
 * snapType, String matcher, String selector, String orderBy) getSnapshot (int
 * snapType, BSONObject matcher, BSONObject selector, BSONObject orderBy)
 * resetSnapshot ()
 * 
 * @author chensiqin
 * @Date 2018.11.21
 * @version 1.00
 */
public class TestSnapshot10371 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String coordAddr;
    private String commCSName;
    private String clName = "cl10371";

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            this.sdb = new Sequoiadb( this.coordAddr, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestSnapshot10371 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestSnapshot10371 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    /**
     * test getSnapshot (int snapType, String matcher, String selector, String
     * orderBy)
     */
    @Test
    public void testGetSnapshotParamString() {
        try {
            DBCursor cursor = null;
            List< BSONObject > dataGroupList = this.getDataGroup(); // get
                                                                    // dataRG

            List< String > actual = new ArrayList< String >();
            List< String > expected = new ArrayList< String >();

            // 0
            cursor = this.sdb.getSnapshot( 0, "", "{Contexts:{$include:1}}",
                    "{\"Contexts\":1}" );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map< String, Object > map = object.toMap();
                Assert.assertEquals( map.keySet().contains( "Contexts" ),
                        true );
            }
            cursor.close();

            cursor = this.sdb.getSnapshot( 0, null,
                    ( BSONObject ) JSON.parse( "{Contexts:{$include:1}}" ),
                    ( BSONObject ) JSON.parse( "{\"Contexts\":1}" ) );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map< String, Object > map = object.toMap();
                Assert.assertEquals( map.keySet().contains( "Contexts" ),
                        true );
            }
            cursor.close();

            // 1
            if ( dataGroupList.size() > 0 && Util.isCluster( this.sdb ) ) {
                BSONObject dataGroup0 = dataGroupList.get( 0 );
                BSONObject group = ( BSONObject ) dataGroup0.get( "Group" );
                String url = this.getUrlByGroupInfo( group );
                Sequoiadb db = new Sequoiadb( url, "", "" );
                cursor = db.getSnapshot( 1, "", "{Contexts:{$include:1}}",
                        "{\"Contexts\":1}" );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    Map< String, Object > map = object.toMap();
                    Assert.assertEquals( map.keySet().contains( "Contexts" ),
                            true );
                }
                cursor.close();
                cursor = db.getSnapshot( 1, null,
                        ( BSONObject ) JSON.parse( "{Contexts:{$include:1}}" ),
                        ( BSONObject ) JSON.parse( "{\"Contexts\":1}" ) );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    Map map = object.toMap();
                    Assert.assertEquals( map.keySet().contains( "Contexts" ),
                            true );
                }
                cursor.close();
                db.disconnect();
            }

            // 2
            actual.clear();
            expected.clear();
            cursor = this.sdb.getSnapshot( 2, "", "{SessionID:{$include:1}}",
                    "{\"SessionID\":1}" );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map map = object.toMap();
                Assert.assertEquals( map.keySet().contains( "SessionID" ),
                        true );
            }
            cursor.close();
            cursor = this.sdb.getSnapshot( 2, null,
                    ( BSONObject ) JSON.parse( "{SessionID:{$include:1}}" ),
                    ( BSONObject ) JSON.parse( "{\"SessionID\":1}" ) );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map map = object.toMap();
                Assert.assertEquals( map.keySet().contains( "SessionID" ),
                        true );
            }
            cursor.close();

            // 3
            if ( dataGroupList.size() > 0 && Util.isCluster( this.sdb ) ) {
                BSONObject dataGroup0 = dataGroupList.get( 0 );
                BSONObject group = ( BSONObject ) dataGroup0.get( "Group" );
                String url = this.getUrlByGroupInfo( group );
                Sequoiadb db = new Sequoiadb( url, "", "" );
                cursor = db.getSnapshot( 3, "", "{SessionID:{$include:1}}",
                        "{\"SessionID\":1}" );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    Map map = object.toMap();
                    Assert.assertEquals( map.keySet().contains( "SessionID" ),
                            true );
                }
                cursor.close();
                cursor = db.getSnapshot( 3, null,
                        ( BSONObject ) JSON.parse( "{SessionID:{$include:1}}" ),
                        ( BSONObject ) JSON.parse( "{\"SessionID\":1}" ) );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    Map map = object.toMap();
                    Assert.assertEquals( map.keySet().contains( "SessionID" ),
                            true );
                }
                cursor.close();
                db.disconnect();
            }

            // 4
            actual.clear();
            String expectedStr = this.commCSName + "." + this.clName;
            cursor = this.sdb
                    .getSnapshot( 4,
                            "{ \"Name\" : \"" + this.commCSName + "."
                                    + this.clName + "\"}",
                            "{Name:{$include:1}}", "" );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actual.add( object.get( "Name" ).toString() );
            }
            cursor.close();
            Assert.assertTrue( actual.contains( expectedStr ) );

            // 5
            actual.clear();
            expectedStr = "";
            cursor = this.sdb.getSnapshot( 5,
                    "{ \"Name\" : \"" + this.commCSName + "\"}",
                    "{Name:{$include:1}}", "" );
            expectedStr = this.commCSName;
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actual.add( object.get( "Name" ).toString() );
            }
            Assert.assertTrue( actual.contains( expectedStr ) );

            // 6
            cursor = this.sdb.getSnapshot( 6, "", "", "" );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map map = object.toMap();
                Assert.assertEquals(
                        map.keySet().contains( "TotalNumConnects" ), true );
            }
            cursor.close();
            cursor = this.sdb.getSnapshot( 6, null, null,
                    ( BSONObject ) JSON.parse( "{\"TotalNumConnects\":1}" ) );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map map = object.toMap();
                Assert.assertEquals(
                        map.keySet().contains( "TotalNumConnects" ), true );
            }
            cursor.close();

            // 7
            cursor = this.sdb.getSnapshot( 7, "", "", "" );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map map = object.toMap();
                Assert.assertEquals( map.keySet().contains( "CPU" ), true );
                Assert.assertEquals( map.keySet().contains( "Memory" ), true );
                Assert.assertEquals( map.keySet().contains( "Disk" ), true );
            }
            cursor.close();
            cursor = this.sdb.getSnapshot( 7, null,
                    ( BSONObject ) JSON.parse( "{\"CPU\":{$include:1}}" ),
                    null );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map map = object.toMap();
                Assert.assertEquals( map.keySet().contains( "CPU" ), true );
            }
            cursor.close();

            // 8
            if ( Util.isCluster( this.sdb ) ) {
                actual.clear();
                expectedStr = "";
                expectedStr = this.commCSName + "." + this.clName;
                cursor = this.sdb.getSnapshot( 8, "", "", "" );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    actual.add( object.get( "Name" ).toString() );
                }
                cursor.close();
                Assert.assertTrue( actual.contains( expectedStr ) );
                cursor = this.sdb.getSnapshot( 8, null,
                        ( BSONObject ) JSON.parse( "{\"Name\":{$include:1}}" ),
                        null );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    actual.add( object.get( "Name" ).toString() );
                }
                cursor.close();
                Assert.assertTrue( actual.contains( expectedStr ) );
            }

        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail(
                    "Sequoiadb driver TestSnapshot10371 testGetSnapshotParamString error, error description:"
                            + e.getMessage() );
        }
    }

    public String getUrlByGroupInfo( BSONObject group ) {
        Map< String, Object > map = group.toMap();
        BSONObject dataObj = this.convertMapToObj( map );
        String hostName = ( String ) dataObj.get( "HostName" );
        map = ( ( BSONObject ) dataObj.get( "Service" ) ).toMap();
        String port = "";
        for ( Map.Entry< String, Object > entry : map.entrySet() ) {
            BSONObject obj = ( BSONObject ) entry.getValue();
            if ( "0".equals( obj.get( "Type" ).toString() ) ) {
                port = ( String ) obj.get( "Name" );
                break;
            }
        }
        return hostName + ":" + port;
    }

    public BSONObject convertMapToObj( Map< String, Object > map ) {
        BSONObject obj = null;
        for ( Map.Entry< String, Object > entry : map.entrySet() ) {
            obj = ( BSONObject ) entry.getValue();
        }
        return obj;
    }

    public List< BSONObject > getDataGroup() {
        List< BSONObject > dataGroupList = new ArrayList< BSONObject >();
        if ( !Util.isCluster( this.sdb ) ) {
            return dataGroupList;
        }
        try {
            List< String > groupList = new ArrayList< String >();
            groupList = this.sdb.getReplicaGroupsInfo();
            for ( String str : groupList ) {
                BSONObject object = ( BSONObject ) JSON.parse( str );
                if ( "0".equals( object.get( "Role" ).toString() ) ) { // datarg
                    dataGroupList.add( object );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestGetList10370 getDataGroup error, error description:"
                            + e.getMessage() );
        }
        return dataGroupList;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
