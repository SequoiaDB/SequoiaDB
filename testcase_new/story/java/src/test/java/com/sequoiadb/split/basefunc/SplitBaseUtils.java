package com.sequoiadb.split.basefunc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbTestException;

public class SplitBaseUtils extends SdbTestBase {
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

    public static ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            throw e;
        }
        return groupList;
    }

    public static DBCollection createHashCl( Sequoiadb db, String clName,
            String dataGroupName ) {
        CollectionSpace cs = db.getCollectionSpace( csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1}, ShardingType:'hash', Group:'"
                        + dataGroupName + "'}" );
        DBCollection cl = cs.createCollection( clName, option );
        return cl;
    }

    public static void checkSplitOnCoord( DBCollection cl,
            List< BSONObject > expRecs ) {
        try {
            List< BSONObject > actRecs = new ArrayList< BSONObject >();
            DBCursor cursor = cl.query( null, null, "{_id:1}", null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actRecs.add( obj );
            }
            cursor.close();
            if ( !actRecs.equals( expRecs ) ) {
                throw new SdbTestException( "data is different" );
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

    public static Sequoiadb getDataDB( Sequoiadb db, String dataGroupName ) {
        String url = db.getReplicaGroup( dataGroupName ).getMaster()
                .getNodeName();
        return new Sequoiadb( url, "", "" );
    }

    public static int checkSplitOnData( Sequoiadb dataDB, String clName,
            int expCnt, int offSet ) {
        try {
            DBCollection cl = dataDB.getCollectionSpace( csName )
                    .getCollection( clName );
            int actCnt = ( int ) cl.getCount();
            if ( Math.abs( actCnt - expCnt ) > offSet ) {
                throw new SdbTestException(
                        "actual count:[" + actCnt + "]" + "excepted count:["
                                + expCnt + "]" + "the split result is wrong" );
            }
            return actCnt;
        } catch ( BaseException e ) {
            throw e;
        } finally {
            dataDB.disconnect();
        }
    }
}