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
import org.testng.annotations.Parameters;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestGetStorageUnits10732 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String commCSName;
    private String clName = "cl10372";

    @BeforeClass
    public void setUp() {
        this.commCSName = SdbTestBase.csName;
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestGetStorageUnits10732 setUp error, error description:"
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
                    "Sequoiadb driver TestGetStorageUnits10732 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    public void testTestGetStorageUnits() throws InterruptedException {
        List< BSONObject > dataGroupList = this.getDataGroup(); // get dataRG
        List< String > actual = new ArrayList< String >();
        if ( dataGroupList.size() > 0 && Util.isCluster( this.sdb ) ) {

            BSONObject dataGroup0 = dataGroupList.get( 0 );
            BSONObject group = ( BSONObject ) dataGroup0.get( "Group" );
            try {
                if ( this.cs.isCollectionExist( clName ) ) {
                    this.cs.dropCollection( clName );
                }
                this.cs.createCollection( clName, ( BSONObject ) JSON.parse(
                        "{Group:\"" + dataGroup0.get( "GroupName" ) + "\"}" ) );
            } catch ( BaseException e ) {
                Assert.fail(
                        "Sequoiadb driver TestGetStorageUnits10732 testTestGetStorageUnits error, error description:"
                                + e.getMessage() );
            }
            // Thread.sleep(100);
            Sequoiadb db = new Sequoiadb( this.getUrlByGroupInfo( group,
                    dataGroup0.get( "PrimaryNode" ) ), "", "" );
            actual.clear();
            actual = db.getStorageUnits();
            db.disconnect();
            Assert.assertTrue( actual.contains( this.commCSName ) == true );
        }
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
                    "Sequoiadb driver TestGetStorageUnits10732 getDataGroup error, error description:"
                            + e.getMessage() );
        }
        return dataGroupList;
    }

    public String getUrlByGroupInfo( BSONObject group, Object primaryNode ) {
        Map< String, Object > map = group.toMap();
        BSONObject dataObj = this.convertMapToObj( map, primaryNode );
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

    public BSONObject convertMapToObj( Map< String, Object > map,
            Object object ) {
        String primaryNode = object.toString();
        BSONObject obj = null;
        for ( Map.Entry< String, Object > entry : map.entrySet() ) {
            obj = ( BSONObject ) entry.getValue();
            if ( primaryNode.equals( obj.get( "NodeID" ).toString() ) ) {
                break;
            }
        }
        return obj;
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
