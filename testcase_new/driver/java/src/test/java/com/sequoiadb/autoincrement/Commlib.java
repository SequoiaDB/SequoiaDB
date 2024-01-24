package com.sequoiadb.autoincrement;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;

public class Commlib {
    public static BSONObject GetAutoIncrement( Sequoiadb sdb,
            String clFullName ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", clFullName );
        BSONObject selector = new BasicBSONObject();
        selector.put( "AutoIncrement", 1 );
        DBCursor cur = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                selector, null );
        BSONObject autoIncrementInfo = new BasicBSONObject();
        while ( cur.hasNext() ) {
            BasicBSONList objs = ( BasicBSONList ) cur.getNext()
                    .get( "AutoIncrement" );
            if ( !objs.isEmpty() ) {
                autoIncrementInfo = ( BasicBSONObject ) objs.get( 0 );
            }
        }
        cur.close();
        return autoIncrementInfo;
    }

    public static List< BSONObject > GetAutoIncrementList( Sequoiadb sdb,
            String clFullName, int autoIncrementCount ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", clFullName );
        BSONObject selector = new BasicBSONObject();
        selector.put( "AutoIncrement", 1 );
        DBCursor cur = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                selector, null );
        List< BSONObject > autoIncrementInfos = new ArrayList<>();
        while ( cur.hasNext() ) {
            BasicBSONList objs = ( BasicBSONList ) cur.getNext()
                    .get( "AutoIncrement" );
            if ( !objs.isEmpty() ) {
                for ( int i = 0; i < autoIncrementCount; i++ ) {
                    autoIncrementInfos.add( ( BasicBSONObject ) objs.get( i ) );
                }
            }
        }
        cur.close();
        return autoIncrementInfos;
    }

    public static BSONObject GetSequenceSnapshot( Sequoiadb sdb,
            String sequenceName ) {
        BSONObject obj = new BasicBSONObject();
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", sequenceName );

        DBCursor cur = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SEQUENCES, matcher,
                null, null );
        while ( cur.hasNext() ) {
            obj = cur.getNext();
        }
        cur.close();
        return obj;
    }
}
