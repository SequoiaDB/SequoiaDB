package com.sequoiadb.crud.truncate;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestException;

public class TruncateUtils {
    public static ArrayList< String > groupList;

    /**
     * create a normal collection
     * 
     * @param sdb
     * @param clName
     * @return DBCollection Object of a normal cl
     * @throws BaseException
     */
    public static DBCollection createCL( Sequoiadb sdb, String csName,
            String clName ) throws BaseException {
        if ( !sdb.isCollectionSpaceExist( csName ) ) {
            sdb.createCollectionSpace( csName );
        }
        DBCollection cl = null;
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
        return cl;
    }

    /**
     * insert data, including some normal records and some Lobs
     * 
     * @param cl
     * @throws BaseException
     */
    public static void insertData( DBCollection cl ) throws BaseException {
        ArrayList< BSONObject > records = new ArrayList< BSONObject >();

        int lobNum = 100;
        ArrayList< ObjectId > oidlist = new ArrayList< ObjectId >();
        for ( int i = 0; i < lobNum; i++ ) {
            oidlist.add( new ObjectId() );
        }
        // put lobs
        String randomStr = "a;kdjflajdfoweine3030asd.f0-:dmalsdf;";
        for ( int i = 0; i < lobNum; i++ ) {
            DBLob lob = null;
            lob = cl.createLob( oidlist.get( i ) );
            lob.write( randomStr.getBytes() );
            lob.close();
        }

        // insert a record with a Lob's oid
        BSONObject record = new BasicBSONObject();
        for ( int i = 0; i < lobNum; i++ ) {
            record = new BasicBSONObject();
            record.put( "name", "zhangsan" + i );
            records.add( record );
        }

        // insert normal records
        int normalRecNum = 100;
        for ( int i = 0; i < normalRecNum; i++ ) {
            record = new BasicBSONObject();
            record.put( "name", "zhangsan" + i );
            record.put( "age", i );
            record.put( "num", i );
            records.add( record );
        }
        cl.insert( records );
    }

    /**
     * check whether truncation is done
     * 
     * @param sdb
     * @param cl
     * @throws BaseException
     */
    public static void checkTruncated( Sequoiadb sdb, DBCollection cl )
            throws BaseException {
        Sequoiadb clGroupDB = null;
        if ( !isStandAlone( sdb ) ) {
            try {
                // get the group of the cl
                String clGroupName = getSrcGroupName( sdb, cl );

                // connect to dataGroup and get information of collection
                String url = sdb.getReplicaGroup( clGroupName ).getMaster()
                        .getNodeName();
                clGroupDB = new Sequoiadb( url, "", "" );

                BSONObject clNameBSON = new BasicBSONObject();
                clNameBSON.put( "Name", cl.getFullName() );
                DBCursor clSnapshot = clGroupDB.getSnapshot( 4, clNameBSON,
                        null, null );
                BasicBSONList clDetails = ( BasicBSONList ) clSnapshot.getNext()
                        .get( "Details" );
                BSONObject clDetail = ( BSONObject ) clDetails.get( 0 );
                clSnapshot.close();

                // justify the information correctness
                boolean dataExist = ( int ) clDetail
                        .get( "TotalDataPages" ) != 0 ? true : false;
                boolean lobExist = ( int ) clDetail.get( "TotalLobPages" ) != 0
                        ? true
                        : false;
                if ( dataExist || lobExist ) {
                    String failMsg = "";
                    if ( dataExist ) {
                        failMsg += "data is still exist!\n";
                    }
                    if ( lobExist ) {
                        failMsg += "lob is still exist!\n";
                    }
                    Assert.fail( failMsg );
                }
            } finally {
                clGroupDB.close();
            }
        } else {
            if ( cl.getCount() != 0 ) {
                Assert.fail( "data is still exist!" );
            }
        }
    }

    /**
     * get the group name of a specific cl
     * 
     * @param sdb
     * @param cl
     * @return groupName
     * @throws BaseException
     */
    public static String getSrcGroupName( Sequoiadb sdb, DBCollection cl )
            throws BaseException {
        // get the snapshot to find the datagroup of cl
        BSONObject clNameBSON = new BasicBSONObject();
        clNameBSON.put( "Name", cl.getFullName() );
        DBCursor gpSnapshot = sdb.getSnapshot( 8, clNameBSON, null, null );

        // extract group information from snapshot
        BasicBSONList CataInfo = ( BasicBSONList ) gpSnapshot.getNext()
                .get( "CataInfo" );
        gpSnapshot.close();
        BasicBSONObject groupInfo = ( BasicBSONObject ) CataInfo.get( 0 );
        String clGroupName = groupInfo.get( "GroupName" ).toString();
        return clGroupName;
    }

    /**
     * get the name of a random destination group
     * 
     * @param sdb
     * @param srcGroupName
     * @return dstGroupName
     * @throws BaseException
     */
    public static String getDstGroupName( Sequoiadb sdb, String srcGroupName )
            throws BaseException {
        // get all groups name
        ArrayList< String > groupNames = sdb.getReplicaGroupNames();
        if ( groupNames.size() < 4 ) {
            throw new SdbTestException( "no groups enough to split" );
        }
        // find a valid destination group
        String dstGroupName = null;
        for ( int i = 0; i < groupNames.size(); i++ ) {
            String currGroupName = groupNames.get( i );
            if ( currGroupName.equals( srcGroupName )
                    || currGroupName.equals( "SYSCatalogGroup" )
                    || currGroupName.equals( "SYSCoord" ) ) {
                continue;
            }
            dstGroupName = currGroupName;
        }
        return dstGroupName;
    }

    public static boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) {
                System.out.printf( "run mode is standalone" );
                return true;
            }
        }
        return false;
    }

    public static boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroups( sdb ).size() < 2 ) {
            System.out.printf( "only one group" );
            return true;
        }
        return false;
    }

    public static ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        groupList = sdb.getReplicaGroupNames();
        groupList.remove( "SYSCatalogGroup" );
        groupList.remove( "SYSCoord" );
        groupList.remove( "SYSSpare" );
        return groupList;
    }
}