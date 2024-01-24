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
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestSetSessionAttr10375 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10375";

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestSetSessionAttr10375 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.cl = this.cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestSetSessionAttr10375 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    public void testSetSessionAttr() {
        try {
            if ( !Util.isCluster( this.sdb ) ) {
                return;
            }
            this.sdb.setSessionAttr(
                    new BasicBSONObject( "PreferedInstance", "M" ) );
            this.cl.query();
            DBCursor cursor = this.sdb.getList(
                    Sequoiadb.SDB_LIST_SESSIONS_CURRENT, null, null, null );
            List< BSONObject > dataGroup = getDataGroup();
            while ( cursor.hasNext() ) {
                BSONObject next = cursor.getNext();
                boolean checkUsedM = checkUsedM( dataGroup,
                        next.get( "NodeName" ).toString() );
                Assert.assertEquals( checkUsedM, true );
            }
        } catch ( BaseException e ) {
            System.out.println( e.getErrorCode() );
            Assert.fail(
                    "Sequoiadb driver TestSetSessionAttr10375 testSetSessionAttr error, error description:"
                            + e.getMessage() );
        }
    }

    public List< BSONObject > getDataGroup() {
        List< BSONObject > dataGroupList = new ArrayList< BSONObject >();
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
                    "Sequoiadb driver TestSetSessionAttr10375 getDataGroup error, error description:"
                            + e.getMessage() );
        }
        return dataGroupList;
    }

    public boolean checkUsedM( List< BSONObject > dataGroup, String nodeName ) {
        String hostName = "";
        String port = "";
        String nodeId = "";
        for ( BSONObject b : dataGroup ) {
            BSONObject group = ( BSONObject ) b.get( "Group" );
            Map< String, Object > map = group.toMap();
            for ( Map.Entry< String, Object > entry : map.entrySet() ) {
                hostName = "";
                nodeId = "";
                BSONObject obj = ( BSONObject ) entry.getValue();
                hostName = ( String ) obj.get( "HostName" );
                nodeId = obj.get( "NodeID" ).toString();
                if ( !nodeId.equals( b.get( "PrimaryNode" ).toString() ) ) {
                    continue;
                }
                Map< String, Object > map2 = ( ( BSONObject ) obj
                        .get( "Service" ) ).toMap();
                for ( Map.Entry< String, Object > entry2 : map2.entrySet() ) {
                    port = "";
                    BSONObject typeObj = ( BSONObject ) entry2.getValue();
                    if ( "0".equals( typeObj.get( "Type" ).toString() ) ) {
                        port = ( String ) typeObj.get( "Name" );
                        if ( nodeName.equals( hostName + ":" + port ) ) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    public BSONObject convertMapToObj( Map< String, Object > map ) {
        BSONObject obj = null;
        for ( Map.Entry< String, Object > entry : map.entrySet() ) {
            obj = ( BSONObject ) entry.getValue();
        }
        return obj;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.sdb.closeAllCursors();
            this.sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
