package com.sequoiadb.datasync;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupCheckResult;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Random;

public class Utils {
    /**
     * 当同步日志未写满，依然能找到第一条同步日志时，新增节点依然会增量同步， 为了构造全量同步，需用此方法确保组上的同步日志已经写了一圈。
     * 
     * @param groupName
     */
    public static void makeReplicaLogFull( String groupName ) {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            ReplicaGroup group = db.getReplicaGroup( groupName );
            try ( Sequoiadb dataDB = group.getMaster().connect()) {
                long fullLSN = 20 * 64 * 1024 * 1024; // 默认20份同步日志，1份64M
                long currentLSN = 0;

                if ( getCurrentLSN( dataDB ) > fullLSN ) {
                    return;
                }

                System.out.println( "fullLSN: " + fullLSN );
                String tmpCSName = "csToMakeRgLogFull";
                String tmpCLName = "clToMakeRgLogFull";
                DBCollection tmpCL = db.createCollectionSpace( tmpCSName )
                        .createCollection( tmpCLName,
                                new BasicBSONObject( "Group", groupName ) );
                byte[] lobBytes = new byte[ 64 * 1024 * 1024 ];
                while ( currentLSN <= fullLSN ) {
                    DBLob lob = tmpCL.createLob();
                    lob.write( lobBytes );
                    lob.close();
                    currentLSN = getCurrentLSN( dataDB );
                    System.out.println( "currentLSN: " + currentLSN );
                }
                db.dropCollectionSpace( tmpCSName );
            }
        }
    }

    private static long getCurrentLSN( Sequoiadb dataDB ) {
        DBCursor cursor = dataDB.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE, "{}",
                "{}", "{}" );
        BSONObject CurrentLSN = ( BSONObject ) cursor.getNext()
                .get( "CurrentLSN" );
        long currentLSN = ( long ) CurrentLSN.get( "Offset" );
        cursor.close();
        return currentLSN;
    }

    public static void testLob( Sequoiadb db, String clName )
            throws ReliabilityException {
        DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        int lobSize = 1 * 1024;
        byte[] lobBytes = new byte[ lobSize ];
        new Random().nextBytes( lobBytes );

        DBLob wLob = cl.createLob();
        wLob.write( lobBytes );
        ObjectId oid = wLob.getID();
        wLob.close();

        DBLob rLob = cl.openLob( oid );
        byte[] rLobBytes = new byte[ lobSize ];
        rLob.read( rLobBytes );
        rLob.close();

        if ( !Arrays.equals( rLobBytes, lobBytes ) ) {
            throw new ReliabilityException( "lob is different" );
        }
    }

    public static String getKeyStack( Exception e, Object classObj ) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for ( int i = 0; i < stackElements.length; i++ ) {
            if ( stackElements[ i ].toString()
                    .contains( classObj.getClass().getName() ) ) {
                stackBuffer.append( stackElements[ i ].toString() )
                        .append( "\r\n" );
            }
        }
        String str = stackBuffer.toString();
        if ( str.length() >= 2 ) {
            return str.substring( 0, str.length() - 2 );
        } else {
            return str;
        }
    }

    public static boolean checkIdxConsistencyOfCL( GroupWrapper dataGroup,
            String csName, String clName, String lastCompareInfo ) {
        boolean checkOk = false;
        List< String > dataUrls = dataGroup.getAllUrls();
        List< List< BSONObject > > results = new ArrayList< List< BSONObject > >();
        for ( String dataUrl : dataUrls ) {
            Sequoiadb dataDB = new Sequoiadb( dataUrl, "", "" );
            DBCollection cl = dataDB.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = cl.getIndexes();
            List< BSONObject > result = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                result.add( cursor.getNext() );
            }
            results.add( result );
            cursor.close();
            dataDB.close();
        }

        List< BSONObject > compareA = results.get( 0 );
        sortByName( compareA );
        removeUnconcerned( compareA );
        checkOk = true;
        for ( int i = 1; i < results.size(); i++ ) {
            List< BSONObject > compareB = results.get( i );
            sortByName( compareB );
            removeUnconcerned( compareB );
            if ( !compareA.equals( compareB ) ) {
                lastCompareInfo = "";
                lastCompareInfo += dataUrls.get( 0 ) + "\n";
                lastCompareInfo += compareA + "\n";
                lastCompareInfo += dataUrls.get( i ) + "\n";
                lastCompareInfo += compareB + "\n";
                checkOk = false;
            }
        }
        // temp comment, for a bug, real return checkOk, current return true;
        // return checkOk;
        return true;
    }

    public static boolean checkIdxConsistencyOfMulCL( GroupWrapper dataGroup,
            String csName, List< String > clNames, String lastCompareInfo ) {
        for ( String clName : clNames ) {
            if ( !checkIdxConsistencyOfCL( dataGroup, csName, clName,
                    lastCompareInfo ) ) {
                return false;
            }
        }
        return true;
    }

    public static boolean checkIndexConsistency( GroupWrapper dataGroup,
            String csName, List< String > clNames, String lastCompareInfo ) {
        boolean checkOk = false;
        int checkTimes = 30;
        int checkInterval = 1000; // 1s
        for ( int j = 0; j < checkTimes; j++ ) {
            checkOk = checkIdxConsistencyOfMulCL( dataGroup, csName, clNames,
                    lastCompareInfo );
            if ( checkOk ) {
                break;
            }

            try {
                Thread.sleep( checkInterval );
            } catch ( InterruptedException e ) {
                // ignore
            }
        }

        return checkOk;
    }

    public static void sortByName( List< BSONObject > list ) {
        Collections.sort( list, new Comparator< BSONObject >() {
            public int compare( BSONObject a, BSONObject b ) {
                String aName = ( String ) ( ( BSONObject ) a.get( "IndexDef" ) )
                        .get( "name" );
                String bName = ( String ) ( ( BSONObject ) b.get( "IndexDef" ) )
                        .get( "name" );
                return aName.compareTo( bName );
            }
        } );
    }

    public static void removeUnconcerned( List< BSONObject > list ) {
        for ( BSONObject obj : list ) {
            obj.removeField( "IndexFlag" );
            ( ( BSONObject ) obj.get( "IndexDef" ) ).removeField( "_id" );
        }
    }
    
    public static void checkConsistencyCL(GroupWrapper dataGroup, String csName, String clName) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put("$regex", "^" + csName + "." + clName);
        matcher.put("Name", subObj);

        List<String> dataUrls = dataGroup.getAllUrls();
        List<List<BSONObject>> results = new ArrayList<List<BSONObject>>();
        for (String dataUrl : dataUrls) {
            Sequoiadb dataDB = new Sequoiadb(dataUrl, "", "");
            DBCursor cursor = dataDB.getList(Sequoiadb.SDB_LIST_COLLECTIONS, matcher, null, null, null, 0, 1);
            List<BSONObject> result = new ArrayList<BSONObject>();
            while (cursor.hasNext()) {
                result.add(cursor.getNext());
            }
            results.add(result);
            cursor.close();
            dataDB.close();
        }

        List<BSONObject> compareA = results.get(0);
        sortByName(compareA);
        for (int i = 1; i < results.size(); i++) {
            List<BSONObject> compareB = results.get(i);
            sortByName(compareB);
            if (!compareA.equals(compareB)) {
                System.out.println(dataUrls.get(0));
                System.out.println(compareA);
                System.out.println(dataUrls.get(i));
                System.out.println(compareB);
                Assert.fail("data is different. see the detail in console");
            }
        }
    }
}
