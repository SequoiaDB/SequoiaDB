package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.List;

import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SubCLUtils {
    public static ArrayList< String > groupList;

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

    public static ArrayList< String > getDataGroups( Sequoiadb sdb )
            throws BaseException {
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            e.printStackTrace();
            throw e;
        }
        return groupList;
    }

    public static List< String > getNodeAddress( Sequoiadb sdb,
            String rgName ) {
        List< String > nodeAddrs = new ArrayList< String >();
        try {
            ReplicaGroup rg = sdb.getReplicaGroup( rgName );
            if ( rg != null ) {
                BasicBSONObject doc = ( BasicBSONObject ) rg.getDetail();
                BasicBSONList groups = ( BasicBSONList ) doc.get( "Group" );
                for ( int i = 0; i < groups.size(); ++i ) {
                    BasicBSONObject group = ( BasicBSONObject ) groups.get( i );
                    String hostName = group.getString( "HostName" );
                    BasicBSONList service = ( BasicBSONList ) group
                            .get( "Service" );
                    BasicBSONObject srcInfo = ( BasicBSONObject ) service
                            .get( 0 );
                    String svcName = srcInfo.getString( "Name" );
                    nodeAddrs.add( hostName + ":" + svcName );
                }
            }
            System.out.println( rgName + " address: " + nodeAddrs.toString() );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Failed to get groupAdrr. ErrorMsg:\n" + e.getMessage() );
        }
        return nodeAddrs;
    }
}
