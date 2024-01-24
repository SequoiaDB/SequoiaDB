package com.sequoiadb.session.accessisolation;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class SessionAccessUtils extends SdbTestBase {

    // insert 1W records
    public static void insertData( DBCollection cl ) {
        List< BSONObject > list = new ArrayList< BSONObject >();
        for ( int i = 0; i < 1000; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse(
                    "{a:" + i + ", test:" + "'testasetatatatatat'" + "}" );
            list.add( obj );
        }
        cl.insert( list, 0 );
    }

    public static void createRGAndNode( Sequoiadb sdb, String groupName,
            Object[] instanceidarr, int nodeNum,
            ConcurrentHashMap< Object, String > instanceidTosvcName ) {
        ReplicaGroup dataRG = sdb.createReplicaGroup( groupName );
        // get hostname
        String tmphostName = sdb.getReplicaGroup( "SYSCatalogGroup" )
                .getMaster().getHostName();
        for ( int i = 0; i < nodeNum; i++ ) {
            int dataPort = SdbTestBase.reservedPortBegin + 100 * i;
            String dataPath = SdbTestBase.reservedDir + "/" + dataPort + "/";
            BSONObject dataConfigue = ( BSONObject ) JSON
                    .parse( "{instanceid :" + instanceidarr[ i ] + "}" );
            boolean checkSucc = false;
            int times = 0;
            int maxRetryTimes = 10;
            do {
                try {
                    dataRG.createNode( tmphostName, dataPort, dataPath,
                            dataConfigue );
                    checkSucc = true;
                } catch ( BaseException e ) {
                    // -145:Node already exists
                    if ( e.getErrorCode() == -145
                            || e.getErrorCode() == -290 ) {
                        dataPort = dataPort + 10;
                        dataPath = SdbTestBase.reservedDir + "/" + dataPort
                                + "/";
                    } else {
                        Assert.fail( "create node fail! port=" + dataPort );
                    }
                }
                times++;
            } while ( !checkSucc && times < maxRetryTimes );

            String svcName = tmphostName + ":" + dataPort;
            instanceidTosvcName.put( instanceidarr[ i ], svcName );
        }
        dataRG.start();
        checkMasterExist( sdb, groupName );
    }

    public static void checkMasterExist( Sequoiadb sdb, String groupName ) {
        int sleepInteval = 10;
        int sleepDuration = 0;
        int maxSleepDuration = 120000;

        String queryMasterNode = "select IsPrimary,NodeName from $SNAPSHOT_SYSTEM where GroupName='"
                + groupName + "' and IsPrimary=true ";
        DBCursor num = sdb.exec( queryMasterNode );
        while ( !num.hasNext() && sleepDuration < maxSleepDuration ) {
            num.close();
            try {
                Thread.sleep( sleepInteval );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            sleepDuration += sleepInteval;
            num = sdb.exec( queryMasterNode );
        }
        num.close();
    }

    public static ArrayList< String > getCoordUrls( Sequoiadb sdb ) {
        DBCursor cursor = null;
        cursor = sdb.getList( Sequoiadb.SDB_LIST_GROUPS, null, null, null );
        ArrayList< String > coordUrls = new ArrayList< String >();

        while ( cursor.hasNext() ) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext();
            String groupName = obj.getString( "GroupName" );
            if ( groupName.equals( "SYSCoord" ) ) {
                BasicBSONList group = ( BasicBSONList ) obj.get( "Group" );
                for ( Object temp : group ) {
                    BSONObject nodeifg = ( BSONObject ) temp;
                    String coordHostName = ( String ) nodeifg.get( "HostName" );
                    BasicBSONList service = ( BasicBSONList ) nodeifg
                            .get( "Service" );
                    BSONObject tempservice = ( BSONObject ) service.get( 0 );
                    String coordPort = ( String ) tempservice.get( "Name" );
                    coordUrls.add( coordHostName + ":" + coordPort );
                }
            }
        }
        cursor.close();
        return coordUrls;
    }

}
