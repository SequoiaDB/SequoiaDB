package com.sequoias3.commlibs3;

import java.util.HashMap;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbStorage implements StorageInterface {
    private static final String TRANSISOLATION = "transisolation";
    private static final String TRANSLOCKWAIT = "translockwait";
    private static final String NODENAME = "NodeName";
    private static Map< String, BSONObject > nodesConf = new HashMap< String, BSONObject >();

    @Override
    public void envPrePare( String coordUrl ) {
        getAllNodeConf( coordUrl );

        // 设置db环境（事务默认开启） ：1、隔离级别为RC 2、设置等待记录锁
        BSONObject configs = new BasicBSONObject();
        configs.put( TRANSISOLATION, 1 );
        configs.put( TRANSLOCKWAIT, true );
        System.out
                .println( "Start preparing for the operating environment..." );
        modifyNodeConf( coordUrl, configs, null );
        System.out.println(
                "Preparing for operation environment is completed...." );
    }

    @Override
    public void envRestore( String coordUrl ) {
        System.out.println( "Start restoring the sequoiadb environment..." );
        for ( String key : nodesConf.keySet() ) {
            BasicBSONObject options = new BasicBSONObject();
            options.put( NODENAME, key );
            modifyNodeConf( coordUrl, nodesConf.get( key ), options );
        }
        System.out.println(
                "Restoring the sequoiadb environment is complete..." );
    }

    @Override
    public String getUrls( String coordUrl ) {
        StringBuffer buf = new StringBuffer();
        try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" )) {
            ReplicaGroup rg = sdb.getReplicaGroup( "SYSCoord" );
            BSONObject rgDetail = rg.getDetail();
            BasicBSONList groupInfo = ( BasicBSONList ) rgDetail.get( "Group" );
            for ( Object gtoupObj : groupInfo.toArray() ) {
                BSONObject groupObj = ( BSONObject ) gtoupObj;
                String hostName = ( String ) groupObj.get( "HostName" );
                BSONObject service = ( BSONObject ) ( ( BasicBSONList ) groupObj
                        .get( "Service" ) ).get( 0 );
                String svcName = ( String ) service.get( "Name" );
                buf.append( hostName + ":" + svcName ).append( "," );
            }
        }
        return buf.substring( 0, buf.length() - 1 );
    }

    @Override
    public String getClusterInfo( String coordUrl ) {
        String info = "";
        try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" ) ;) {
            DBCursor cur = db.getList( 7, null, null, null );
            while ( cur.hasNext() ) {
                info += cur.getNext().toString();
            }
            cur.close();
        }
        return info;
    }

    private static void getAllNodeConf( String coordUrl ) {
        BasicBSONObject selector = new BasicBSONObject();
        try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" )) {
            selector.put( NODENAME, "" );
            selector.put( TRANSISOLATION, "" );
            selector.put( TRANSLOCKWAIT, "" );

            DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                    selector, null );
            while ( cursor.hasNext() ) {
                BasicBSONObject doc = ( BasicBSONObject ) cursor.getNext();
                String key = doc.getString( NODENAME );
                doc.remove( NODENAME );
                nodesConf.put( key, doc );
            }
            cursor.close();
        }
    }

    private static void modifyNodeConf( String coordUrl, BSONObject configs,
            BSONObject options ) {
        if ( options == null ) {
            options = new BasicBSONObject().append( "Global", true );
        }

        try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" )) {
            sdb.updateConfig( configs, options );
        } catch ( BaseException e ) {
            e.printStackTrace();
            throw e;
        }
    }
}
