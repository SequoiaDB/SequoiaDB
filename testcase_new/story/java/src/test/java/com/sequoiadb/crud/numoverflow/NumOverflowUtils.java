package com.sequoiadb.crud.numoverflow;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class NumOverflowUtils {
    public static ArrayList< String > groupList;

    /**
     * Judge the mode
     * 
     * @param sdb
     * @return true/false, true is standalone, false is cluster
     */
    public static boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) {
                System.out.printf( "The mode is standalone." );
                return true;
            }
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
            Assert.assertTrue( false, "getDataGroups fail " + e.getMessage() );
        }
        return groupList;
    }

    public static boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroups( sdb ).size() < 2 ) {
            System.out.printf( "only one group" );
            return true;
        }
        return false;
    }

    public static String getSourceRGName( Sequoiadb sdb, String csName,
            String clName ) {
        String groupName = "";
        String cond = String.format( "{Name:\"%s.%s\"}", csName, clName );
        DBCursor collections = sdb.getSnapshot( 8, cond, null, null );
        while ( collections.hasNext() ) {
            BSONObject collection = collections.getNext();
            BasicBSONObject doc = ( BasicBSONObject ) collection;
            doc.getString( "Name" );
            BasicBSONList subdoc = ( BasicBSONList ) doc.get( "CataInfo" );
            BasicBSONObject elem = ( BasicBSONObject ) subdoc.get( 0 );
            groupName = elem.getString( "GroupName" );
        }
        return groupName;
    }

    public static String getTarRgName( Sequoiadb sdb, String sourceRGName ) {
        String tarRgName = "";
        for ( int i = 0; i < groupList.size(); ++i ) {
            String name = groupList.get( i );
            if ( !name.equals( sourceRGName ) ) {
                tarRgName = name;
                break;
            }
        }
        return tarRgName;
    }

    public static DBCollection createCL( CollectionSpace cs, String clName,
            String option ) {
        DBCollection cl = null;
        BSONObject options = ( BSONObject ) JSON.parse( option );
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }

            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    public static DBCollection createCL( CollectionSpace cs, String clName ) {
        DBCollection cl = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }

            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    public static void insert( DBCollection cl, String[] records ) {
        try {
            for ( int i = 0; i < records.length; i++ ) {
                cl.insert( records[ i ] );
            }
        } catch ( BaseException e ) {
            Assert.fail( "insert datas failed, errMsg:" + e.getMessage()
                    + e.getErrorCode() );
        }
    }

    /**
     * arithmetic operation as a selector,and check the overflow value type
     * 
     * @param: cl:rg:"db.cs.cl"
     *             matcherValue: used to match a record sValue: selector
     *             operator,rg:{"$abs":1} selectorName: field name to
     *             participate in an operation,rg:{a:1},the selectorName is "a"
     *             isVerifyDataType: if true validation data type as required,if
     *             false not validation data type
     */
    public static void selectorOper( DBCollection cl, int matcherValue,
            BSONObject sValue, String selectorName, String[] expRecords ) {
        try {
            DBQuery query = new DBQuery();
            BSONObject selector = new BasicBSONObject();
            BSONObject sValue1 = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            matcher.put( "test", matcherValue );
            sValue1.put( "$include", 0 );
            selector.put( selectorName, sValue );
            selector.put( "_id", sValue1 );
            query.setSelector( selector );
            query.setMatcher( matcher );
            DBCursor cursor = cl.query( query );
            List< BSONObject > actualList = new ArrayList< BSONObject >();

            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();

            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 0; i < expRecords.length; i++ ) {
                BSONObject expRecord = ( BSONObject ) JSON
                        .parse( expRecords[ i ] );
                expectedList.add( expRecord );
            }
            Assert.assertEquals( actualList, expectedList,
                    "the actual query datas is error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "operator is used as selector oper failed,"
                            + e.getMessage() );
        }
    }

    /**
     * Check strict data control mode
     * 
     * @param: cl
     *             operSymbol: inculde $abs/$add/$substract/$multiply/$divide
     *             selectorName: field name to participate in an
     *             operation,rg:{a:1},the selectorName is "a" arithmeticValue:
     *             the value involved in the operation,rg:a+1,and the 1 is
     *             arithmeticValue
     */
    public static void isStrictDataTypeOper( DBCollection cl,
            BSONObject selector ) {
        try {
            DBQuery query = new DBQuery();
            BSONObject matcher = new BasicBSONObject();
            query.setSelector( selector );
            query.setMatcher( matcher );
            DBCursor cursor = cl.query( query );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                System.out.println( "actRecs==" + object.toString() );
            }
            cursor.close();
            Assert.fail( "the operation must be error!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -318 ) {
                Assert.assertTrue( false, "oper should be failed,"
                        + e.getErrorCode() + e.getMessage() );
            }
        }
    }

    /**
     * Check strict data control mode by update
     * 
     * @param: cl
     *             operSymbol: $inc updateValue: update value,rg:{a:1}
     */
    public static void updateIsStrictDataType( DBCollection cl,
            BSONObject updateValue ) {
        try {
            BSONObject modifier = new BasicBSONObject();
            modifier.put( "$inc", updateValue );
            cl.update( null, modifier, null,
                    DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY );

            // if there is no error ,then query the data
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            DBCursor cursor = cl.query( null );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();
            Assert.fail( "the operation must be error!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -318 ) {
                Assert.assertTrue( false, "update is overflow should be failed,"
                        + e.getErrorCode() + e.getMessage() );
            }
        }
    }

    /**
     * Check strict data control mode by update
     * 
     * @param: cl
     *             operSymbol: $inc upsertValue: update value,rg:{a:1}
     */
    public static void upsertIsStrictDataType( DBCollection cl,
            BSONObject upsertValue, BSONObject matherValue ) {
        try {
            BSONObject modifier = new BasicBSONObject();
            modifier.put( "$inc", upsertValue );
            cl.upsert( matherValue, modifier, null );
            // if there is no error ,then query the data
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            DBCursor cursor = cl.query( null );
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();
            Assert.fail( "the operation must be error!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -318 ) {
                Assert.assertTrue( false, "upsert is overflow should be failed,"
                        + e.getErrorCode() + e.getMessage() );
            }
        }
    }

    public static void multipleFieldOper( DBCollection cl, String selector,
            String[] expRecords ) {
        try {
            long skipRows = 0;
            long returnRows = -1;
            DBCursor cursor = cl.query( null, selector, null, null, skipRows,
                    returnRows );

            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }

            cursor.close();

            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 0; i < expRecords.length; i++ ) {
                BSONObject expRecord = ( BSONObject ) JSON
                        .parse( expRecords[ i ] );
                expectedList.add( expRecord );
            }

            Assert.assertEquals( actualList, expectedList,
                    "the actual query datas is error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "perform arithmetic error,"
                    + e.getErrorCode() + e.getMessage() );
        }
    }

    public static void checkDataType( DBCollection cl, BSONObject sValue,
            int matcherValue, String selectorName, String expTypeToSdb,
            Boolean isVerifyTypeToJava, String expTypeToJava )
            throws Exception {
        try {
            if ( !selectorName.contains( "$" ) ) {
                DBQuery query = new DBQuery();
                DBQuery query1 = new DBQuery();
                BSONObject selector = new BasicBSONObject();
                BSONObject selector1 = new BasicBSONObject();
                BSONObject matcher = new BasicBSONObject();
                matcher.put( "test", matcherValue );
                selector1.put( selectorName,
                        new BasicBSONObject( sValue.toMap() ) );
                sValue.put( "$type", 2 );
                selector.put( selectorName, sValue );
                query.setSelector( selector );
                query.setMatcher( matcher );
                DBCursor cursor = cl.query( query );

                String type = "";
                BSONObject object = null;
                while ( cursor.hasNext() ) {
                    object = cursor.getNext();
                    BasicBSONObject obj = ( BasicBSONObject ) object;
                    type = getType( selectorName, obj );

                }
                cursor.close();
                Assert.assertEquals( type, expTypeToSdb,
                        "the numtype is error" );

                // check the data type from java client
                if ( isVerifyTypeToJava ) {
                    String typeOfJava = "";
                    BSONObject object1 = null;
                    query1.setSelector( selector1 );
                    query1.setMatcher( matcher );
                    DBCursor cursor1 = cl.query( query1 );
                    while ( cursor1.hasNext() ) {
                        object1 = cursor1.getNext();
                        typeOfJava = object1.get( selectorName ).getClass()
                                .toString();
                    }
                    cursor1.close();
                    Assert.assertEquals( typeOfJava, expTypeToJava,
                            "the numtype from java is error" );
                }
            }

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "check the data type failed," + e.getMessage() );
        }
    }

    public static String getType( String inField, BasicBSONObject object )
            throws Exception {

        String[] tmps = inField.split( "\\." );
        String type = null;
        if ( tmps.length == 1 ) {
            if ( object.containsField( tmps[ 0 ] ) ) {
                type = ( ( BasicBSONObject ) object ).getString( tmps[ 0 ] );
            }
        } else {
            BSONObject subobj = object;
            for ( int i = 0; i < tmps.length - 1; ++i ) {
                if ( subobj.containsField( tmps[ i ] ) ) {
                    subobj = ( BSONObject ) subobj.get( tmps[ i ] );
                } else {
                    throw new Exception(
                            object.toString() + "not contain:" + tmps[ i ] );
                }
            }
            if ( tmps[ tmps.length - 1 ].contains( "$[0]" ) ) {
                type = ( String ) ( ( BasicBSONList ) subobj ).get( 0 );
            } else {
                if ( subobj.containsField( tmps[ tmps.length - 1 ] ) ) {
                    type = ( ( BasicBSONObject ) subobj )
                            .getString( tmps[ tmps.length - 1 ] );
                } else {
                    throw new Exception( object.toString() + "not contain:"
                            + tmps[ tmps.length - 1 ] );
                }
            }
        }

        return type;

    }

    /**
     * arithmetic operation as a matcher,and check the overflow value type
     * 
     * @param: cl:rg:"db.cs.cl"
     *             matcherName: matcher field name matcherValue: matcher
     *             operator,rg:{"$abs":1,"$et":123} *
     */
    public static void matcherOper( DBCollection cl, String matcherName,
            BSONObject matcherValue, String[] expRecords ) {
        try {
            DBQuery query = new DBQuery();
            BSONObject selector = new BasicBSONObject();
            BSONObject sValue = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject sort = new BasicBSONObject();
            sValue.put( "$include", 0 );
            selector.put( "_id", sValue );
            matcher.put( matcherName, matcherValue );
            sort.put( "test", 1 );
            query.setMatcher( matcher );
            query.setSelector( selector );
            query.setOrderBy( sort );
            DBCursor cursor = cl.query( query );
            System.out.println( "matcher====:" + matcher.toString() );
            List< BSONObject > actualList = new ArrayList< BSONObject >();

            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();

            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 0; i < expRecords.length; i++ ) {
                BSONObject expRecord = ( BSONObject ) JSON
                        .parse( expRecords[ i ] );
                expectedList.add( expRecord );
            }
            Assert.assertEquals( actualList, expectedList,
                    "the actual query datas is error" );

            // create index, and query by index scans
            String indexName = createIndex( cl, matcherName );
            BSONObject hint = new BasicBSONObject();
            hint.put( "", indexName );
            query.setHint( hint );
            DBCursor cursor1 = cl.query( query );
            List< BSONObject > actualList1 = new ArrayList< BSONObject >();
            while ( cursor1.hasNext() ) {
                BSONObject object1 = cursor1.getNext();
                actualList1.add( object1 );
            }
            cursor1.close();
            Assert.assertEquals( actualList1, actualList,
                    "index and table scans query is different!" );
            dropIndex( cl, indexName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "operator is used as matcher oper failed,"
                    + e.getMessage() );
        }
    }

    public static String createIndex( DBCollection cl, String keyName ) {
        // create index
        String indexName = "testIndex";
        boolean isUnique = false;
        boolean enforced = false;
        int sortBufferSize = 64;
        BSONObject indexObj = new BasicBSONObject();

        try {
            indexObj.put( keyName, 1 );
            cl.createIndex( indexName, indexObj, isUnique, enforced,
                    sortBufferSize );
        } catch ( BaseException e ) {
            Assert.fail(
                    "create index fail:" + e.getErrorCode() + e.getMessage() );
        }
        return indexName;
    }

    public static void dropIndex( DBCollection cl, String indexName ) {
        // drop index
        try {
            cl.dropIndex( indexName );
        } catch ( BaseException e ) {
            Assert.fail(
                    "dropIndex fail:" + e.getErrorCode() + e.getMessage() );
        }

    }

    public static void multiFieldOperAsMatcher( DBCollection cl, String matcher,
            String[] expRecords, String indexKey ) {
        try {
            long skipRows = 0;
            long returnRows = -1;
            String selector = "{_id:{$include:0}}";
            DBCursor cursor = cl.query( matcher, selector, null, null, skipRows,
                    returnRows );

            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();

            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 0; i < expRecords.length; i++ ) {
                BSONObject expRecord = ( BSONObject ) JSON
                        .parse( expRecords[ i ] );
                expectedList.add( expRecord );
            }

            Assert.assertEquals( actualList, expectedList,
                    "the actual query datas is error" );

            // create index
            String indexName = "mutilIndex";
            try {
                int sortBufferSize = 64;
                cl.createIndex( indexName, indexKey, false, false,
                        sortBufferSize );
            } catch ( BaseException e ) {
                Assert.fail( "create index fail:" + e.getErrorCode()
                        + e.getMessage() );
            }

            // query by index
            String hint = "{'':'nutilIndex'}";
            DBCursor cursor1 = cl.query( matcher, selector, null, hint,
                    skipRows, returnRows );
            List< BSONObject > actualList1 = new ArrayList< BSONObject >();
            while ( cursor1.hasNext() ) {
                BSONObject object1 = cursor1.getNext();
                actualList1.add( object1 );
            }
            cursor1.close();
            Assert.assertEquals( actualList1, actualList,
                    "index and table scans query is different!" );

        } catch ( BaseException e ) {
            Assert.assertTrue( false, "perform arithmetic error,"
                    + e.getErrorCode() + e.getMessage() );
        }
    }

    /**
     * arithmetic operation as a selector,and check the overflow value type
     * 
     * @param: cl:rg:"db.cs.cl"
     *             matcherValue: used to match a record sValue: selector
     *             operator,rg:{"$abs":1} selectorName: field name to
     *             participate in an operation,rg:{a:1},the selectorName is "a"
     *             isVerifyDataType: if true validation data type as required,if
     *             false not validation data type
     */
    public static void updateOper( DBCollection cl, int matcherValue,
            BSONObject updateValue, String operate ) {
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject modifier = new BasicBSONObject();
            matcher.put( "test", matcherValue );
            modifier.put( "$inc", updateValue );

            if ( operate == "updateShardingKey" ) {
                cl.update( matcher, modifier, null,
                        DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY );
            } else if ( operate == "update" ) {
                cl.update( matcher, modifier, null );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "update is oper failed," + e.getMessage() );
        }
    }

    /**
     * upsert use $inc,,and check the overflow value type
     * 
     * @param: cl:rg:"db.cs.cl"
     *             matcher: used to match a record,rg:{a:1} updateValue:
     *             rg:{"b":3} *
     */
    public static void upsertOper( DBCollection cl, BSONObject matcher,
            BSONObject updateValue, int setInsertValue, String operate ) {
        try {
            BSONObject modifier = new BasicBSONObject();
            BSONObject setOnInsert = new BasicBSONObject();
            modifier.put( "$inc", updateValue );
            setOnInsert.put( "test", setInsertValue );

            if ( operate == "upsert" ) {
                cl.upsert( matcher, modifier, null, setOnInsert );
            } else if ( operate == "upsertShardingKey" ) {
                cl.upsert( matcher, modifier, null, setOnInsert,
                        DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "update is oper failed," + e.getMessage() );
        }
    }

    /**
     * check update data
     * 
     * @param: matcherValue:
     *             used to match a record
     */
    public static void checkUpdateResult( DBCollection cl, int matcherValue,
            String[] expRecords ) {
        try {
            DBQuery query = new DBQuery();
            BSONObject selector = new BasicBSONObject();
            BSONObject sValue = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            matcher.put( "test", matcherValue );
            sValue.put( "$include", 0 );
            selector.put( "_id", sValue );
            query.setSelector( selector );
            query.setMatcher( matcher );

            DBCursor cursor = cl.query( query );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();

            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 0; i < expRecords.length; i++ ) {
                BSONObject expRecord = ( BSONObject ) JSON
                        .parse( expRecords[ i ] );
                expectedList.add( expRecord );
            }
            Assert.assertEquals( actualList, expectedList,
                    "the actual query datas is error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "query data of update failed," + e.getMessage() );
        }
    }

    /**
     * check update data type
     * 
     * @param: matcherValue:
     *             used to match a record updateName: update fieldName
     *             expTypeToSdb: query expected data type by sdb,check the data
     *             Type isVerifyTypeToJava:is or not check the data type by java
     *             Client,true/false expTypeToJava: query expected data type by
     *             java Client
     */
    public static void checkUpdateDataType( DBCollection cl, int matcherValue,
            String updateName, String expTypeToSdb, Boolean isVerifyTypeToJava,
            String expTypeToJava ) throws Exception {
        try {
            if ( !updateName.contains( "$" ) ) {
                DBQuery query = new DBQuery();
                DBQuery query1 = new DBQuery();
                BSONObject selector = new BasicBSONObject();
                BSONObject sValue = new BasicBSONObject();
                BSONObject matcher = new BasicBSONObject();
                matcher.put( "test", matcherValue );
                sValue.put( "$type", 2 );
                selector.put( updateName, sValue );
                query.setSelector( selector );
                query.setMatcher( matcher );

                DBCursor cursor = cl.query( query );
                String type = "";
                BSONObject object = null;
                while ( cursor.hasNext() ) {
                    object = cursor.getNext();
                    BasicBSONObject obj = ( BasicBSONObject ) object;
                    type = getType( updateName, obj );
                }
                cursor.close();
                Assert.assertEquals( type, expTypeToSdb,
                        "the numtype is error" );

                // check the data type from java client
                if ( isVerifyTypeToJava ) {
                    String typeOfJava = "";
                    BSONObject object1 = null;
                    query1.setMatcher( matcher );
                    DBCursor cursor1 = cl.query( query1 );
                    while ( cursor1.hasNext() ) {
                        object1 = cursor1.getNext();
                        typeOfJava = object1.get( updateName ).getClass()
                                .toString();
                    }
                    cursor1.close();
                    Assert.assertEquals( typeOfJava, expTypeToJava,
                            "the numtype from java is error" );
                }
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "check the data type failed," + e.getMessage() );
        }
    }

    /**
     * sql operation by sdb
     * 
     * @param: sql:
     *             SQL,rg:"select a+1 from cs.cl where test=1" *
     */
    public static void sqlOper( Sequoiadb sdb, String sql,
            String[] expRecords ) {
        DBCursor cursor = sdb.exec( sql );
        List< BSONObject > actualList = new ArrayList< BSONObject >();

        while ( cursor.hasNext() ) {
            BSONObject object = cursor.getNext();
            actualList.add( object );
        }
        cursor.close();

        List< BSONObject > expectedList = new ArrayList< BSONObject >();
        for ( int i = 0; i < expRecords.length; i++ ) {
            BSONObject expRecord = ( BSONObject ) JSON.parse( expRecords[ i ] );
            expectedList.add( expRecord );
        }
        Assert.assertEquals( actualList, expectedList,
                "the actual query datas is error" );
    }
}
