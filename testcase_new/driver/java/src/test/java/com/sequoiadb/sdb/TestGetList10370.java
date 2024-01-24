package com.sequoiadb.sdb;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestGetList10370 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String commCSName;
    private String clName = "cl10370";

    @BeforeClass
    public void setUp() {
        this.commCSName = SdbTestBase.csName;
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestGetList10370 setUp error, error description:"
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
                    "Sequoiadb driver TestGetList10370 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    public void testGetList() throws InterruptedException {
        try {
            DBCursor cursor = null;
            List< BSONObject > dataGroupList = this.getDataGroup(); // get
                                                                    // dataRG
            // 0
            cursor = this.sdb.getList( 0, null,
                    ( BSONObject ) JSON.parse( "{Contexts:{$include:1}}" ),
                    ( BSONObject ) JSON.parse( "{\"Contexts\":1}" ) );
            List< String > actual = new ArrayList< String >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                Map map = object.toMap();
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
                cursor = db.getList( 1, null,
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
            cursor = this.sdb.getList( 2, null,
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
                cursor = db.getList( 3, null,
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
            String expectedStr = "{ \"Name\" : \"" + this.commCSName + "."
                    + this.clName + "\" }";
            cursor = this.sdb.getList( 4,
                    ( BSONObject ) JSON.parse( expectedStr ),
                    ( BSONObject ) JSON.parse( "{Name:{$include:1}}" ),
                    ( BSONObject ) JSON.parse( "{\"Name\":1}" ) );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actual.add( object.toString() );
            }
            cursor.close();
            Assert.assertTrue( actual.contains( expectedStr ) );
            cursor = this.sdb.getSnapshot( 4,
                    ( BSONObject ) JSON.parse( expectedStr ),
                    ( BSONObject ) JSON.parse( "{Name:{$include:1}}" ),
                    ( BSONObject ) JSON.parse( "{\"Name\":1}" ) );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actual.add( object.toString() );
            }
            cursor.close();
            Assert.assertTrue( actual.contains( expectedStr ) );

            // 5
            actual.clear();
            expectedStr = "";
            expectedStr = "{ \"Name\" : \"" + this.commCSName + "\" }";
            cursor = this.sdb.getList( 5,
                    ( BSONObject ) JSON.parse( expectedStr ),
                    ( BSONObject ) JSON.parse( "{Name:{$include:1}}" ),
                    ( BSONObject ) JSON.parse( "{\"Name\":1}" ) );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actual.add( object.toString() );
            }
            cursor.close();
            Assert.assertTrue( actual.contains( expectedStr ) );
            cursor = this.sdb.getSnapshot( 5,
                    ( BSONObject ) JSON.parse( expectedStr ),
                    ( BSONObject ) JSON.parse( "{Name:{$include:1}}" ),
                    ( BSONObject ) JSON.parse( "{\"Name\":1}" ) );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actual.add( object.toString() );
            }
            cursor.close();
            Assert.assertTrue( actual.contains( expectedStr ) );

            // 6
            if ( dataGroupList.size() > 0 && Util.isCluster( this.sdb ) ) {
                BSONObject dataGroup0 = dataGroupList.get( 0 );
                BSONObject group = ( BSONObject ) dataGroup0.get( "Group" );
                String url = this.getUrlByGroupInfo( group );
                try {
                    if ( this.cs.isCollectionExist( clName ) ) {
                        this.cs.dropCollection( clName );
                    }
                    this.cs.createCollection( clName,
                            ( BSONObject ) JSON.parse( "{Group:\""
                                    + dataGroup0.get( "GroupName" ) + "\"}" ) );
                } catch ( BaseException e ) {
                    e.printStackTrace();
                    Assert.fail(
                            "Sequoiadb driver TestGetList10370 testGetList type 6 error, error description:"
                                    + e.getMessage() );
                }
                // Thread.sleep(100);
                Sequoiadb db = new Sequoiadb( url, "", "" );
                cursor = db.getList( 6, null, null, null );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    Map map = object.toMap();
                    actual.add( object.get( "Name" ).toString() );
                }
                cursor.close();
                Assert.assertTrue( actual.contains( expectedStr ) );
                db.disconnect();

            }

            // 7
            if ( Util.isCluster( this.sdb ) ) {
                cursor = this.sdb.getList( 7, null, null, null );
                while ( cursor.hasNext() ) {
                    BSONObject object = cursor.getNext();
                    Map map = object.toMap();
                    Assert.assertEquals( map.keySet().contains( "Group" ),
                            true );
                }
                cursor.close();
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail(
                    "Sequoiadb driver TestGetList10370 testGetList error, error description:"
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
