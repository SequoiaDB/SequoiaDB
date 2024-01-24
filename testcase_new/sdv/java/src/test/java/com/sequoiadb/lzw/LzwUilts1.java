package com.sequoiadb.lzw;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class LzwUilts1 {

    /**
     * Judge the mode
     * 
     * @param sdb
     * @return true/false, true is standalone, false is cluster
     */
    public boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) { // -159:The operation is for coord
                                              // node only
                // System.out.printf("The mode is standalone.");
                return true;
            }
        }
        return false;
    }

    public static List< String > getDataRgNames( Sequoiadb sdb ) {
        List< String > rgNames = new ArrayList< String >();
        try {
            rgNames = sdb.getReplicaGroupNames();
            rgNames.remove( "SYSCatalogGroup" );
            rgNames.remove( "SYSCoord" );
            rgNames.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return rgNames;
    }

    public static DBCollection createCL( CollectionSpace cs, String clName,
            BSONObject option ) {
        DBCollection cl = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    public void insertData( DBCollection cl, int start, int recSum,
            int strLength ) {
        try {
            // List<BSONObject> recs = new ArrayList<BSONObject>();
            for ( int i = start; i < recSum; i++ ) {
                BSONObject rec = new BasicBSONObject();
                rec.put( "_id", i );
                rec.put( "age", i );
                rec.put( "num", i );
                rec.put( "str", getRandomString( strLength ) );
                rec.put( "decimal", new BSONDecimal( "123.456" ) );
                rec.put( "decimal", new BSONDecimal( "1." + getDecimal999() ) );
                // recs.add(rec);
                cl.insert( rec );
            }
            // cl.bulkInsert(recs, 0);
        } catch ( BaseException e ) {
            throw e;
        }
    }

    private String getDecimal999() {
        String str = "";
        for ( int i = 1; i <= 999; i++ ) {
            str += "1";
        }
        return str;
    }

    private String getRandomString( int length ) {
        String base = "abc";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int index = random.nextInt( base.length() );
            sb.append( base.charAt( index ) );
        }
        return sb.toString();
    }

    public static String getGroupIPByGroupName( Sequoiadb sdb,
            String groupName ) {
        String dataUrl = "";
        try {
            ReplicaGroup replicaGroup = sdb.getReplicaGroup( groupName );
            Node master = replicaGroup.getMaster();
            dataUrl = master.getNodeName();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return dataUrl;
    }
}
