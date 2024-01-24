package com.sequoiadb.backup;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class BackupUtil extends SdbTestBase {
    public static void createRGAndNode( Sequoiadb sdb, String groupName,
            int nodeNum ) {
        ReplicaGroup dataRG = sdb.createReplicaGroup( groupName );
        // get hostname
        String tmphostName = sdb.getReplicaGroup( "SYSCatalogGroup" )
                .getMaster().getHostName();
        for ( int i = 0; i < nodeNum; i++ ) {
            int dataPort = SdbTestBase.reservedPortBegin + 100 * i;
            String dataPath = SdbTestBase.reservedDir + "/" + dataPort + "/";
            boolean checkSucc = false;
            int times = 0;
            int maxRetryTimes = 10;
            do {
                try {
                    dataRG.createNode( tmphostName, dataPort, dataPath );
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
        }
        dataRG.start();
    }

    public static void insertData( DBCollection cl ) {
        List< BSONObject > list = new ArrayList< BSONObject >();
        for ( int i = 0; i < 1000; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse(
                    "{a:" + i + ", test:" + "'testasetatatatatat'" + "}" );
            list.add( obj );
        }
        cl.insert( list );
    }

    public static void removeFile( File file ) {
        if ( file.exists() ) {
            if ( file.isFile() ) {
                file.delete();
            } else {
                File[] files = file.listFiles();
                for ( File subFile : files ) {
                    removeFile( subFile );
                }

                file.delete();
            }
        }
    }
}
