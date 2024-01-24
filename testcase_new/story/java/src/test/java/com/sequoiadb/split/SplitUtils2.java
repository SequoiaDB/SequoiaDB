package com.sequoiadb.split;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SplitUtils2 {

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

    public static List< BSONObject > insertData( DBCollection cl, int len ) {
        List< BSONObject > insertRecods = new ArrayList< BSONObject >();
        try {
            BSONObject bson = null;
            for ( int i = 1; i < len; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "str", i + "abc" );
                bson.put( "age", i );
                bson.put( "num", ( i * 1.0 ) / 100 );
                long height = i;
                bson.put( "height", height );
                boolean boolvalue = ( i % 2 == 0 ) ? true : false;
                if ( i == 1 || i == 2 ) {

                } else if ( i == 3 || i == 4 ) {
                    bson.put( "boolvalue", i );
                } else {
                    bson.put( "boolvalue", boolvalue );
                }

                SimpleDateFormat sdf = new SimpleDateFormat( "yyyy-MM-dd" );
                Date date = null;
                String str = i + "";
                try {
                    if ( i < 10 ) {
                        str = "0" + str;
                    }
                    date = sdf.parse( "2016-12-" + str );
                } catch ( ParseException e ) {
                    e.printStackTrace();
                }
                bson.put( "date", date );
                // String mydate = "2014-07-01 12:30:30." + 1111111;
                str = "2016-12-0" + i % 2;
                str = str + " 12:30:0" + i % 10 + "."
                        + ( 100000 + ( 123456 + i ) % 100000 );
                String mydate = str;
                String dateStr = mydate.substring( 0,
                        mydate.lastIndexOf( '.' ) );
                String incStr = mydate
                        .substring( mydate.lastIndexOf( '.' ) + 1 );
                SimpleDateFormat format = new SimpleDateFormat(
                        "yyyy-MM-dd HH:mm:ss" );
                try {
                    date = format.parse( dateStr );
                } catch ( ParseException e ) {
                    e.printStackTrace();
                }
                int seconds = ( int ) ( date.getTime() / 1000 );
                int inc = Integer.parseInt( incStr );
                BSONTimestamp ts = new BSONTimestamp( seconds, inc );
                bson.put( "timestamp", ts );

                str = "12345.06789123456789012345" + i * 100;
                BSONDecimal decimal = new BSONDecimal( str );
                bson.put( "bdecimal", decimal );

                BSONObject subObj = new BasicBSONObject();
                subObj.put( "a", i );
                bson.put( "subobj", subObj );

                BSONObject arr = new BasicBSONList();
                arr.put( "0", i );
                bson.put( "arrtype", arr );

                if ( i == 4 || i == 6 ) {
                    // 不插入nulltype类型
                } else if ( i % 2 == 0 ) {
                    bson.put( "nulltype", null );
                } else {
                    bson.put( "nulltype", i );
                }
                insertRecods.add( bson );
            }

            cl.bulkInsert( insertRecods, 0 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail();
        }
        return insertRecods;
    }

}
