package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class SQL7108 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        String csName = "cs7108";
        String clName = "cl7108";

        // create cs and cl
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            sdb.execUpdate( "create collectionspace " + csName );
            sdb.execUpdate( "create collection " + csName + "." + clName );

            int i = 0;
            DBCursor csCursor = sdb.exec( "list collectionspaces" );
            while ( csCursor.hasNext() ) {
                String actualCSName = ( String ) csCursor.getNext()
                        .get( "Name" );
                if ( actualCSName.equals( csName ) ) {
                    i++;
                }
            }
            Assert.assertEquals( i, 1 );

            int j = 0;
            DBCursor clCursor = sdb.exec( "list collections" );
            while ( clCursor.hasNext() ) {
                String actualCLName = ( String ) clCursor.getNext()
                        .get( "Name" );
                if ( actualCLName.equals( csName + "." + clName ) ) {
                    j++;
                }
            }
            Assert.assertEquals( j, 1 );
        } catch ( BaseException e ) {
            Assert.fail( "create cs or cl failed, errMsg:" + e.getMessage() );
        }

        // insert and update data
        int nameNum = 2;
        int numForOneName = 5;
        String name = "test";
        ArrayList< BSONObject > expectDataList = new ArrayList< BSONObject >();
        BSONObject obj = null;
        try {
            for ( int i = 0; i < nameNum; i++ ) {
                for ( int j = 0; j < numForOneName; j++ ) {
                    int age = 20;
                    age = age + j;
                    String insertData = "insert into " + csName + "." + clName
                            + "(name,age) " + "values(" + "'" + name + i + "'"
                            + "," + age + ")";
                    sdb.execUpdate( insertData );
                    obj = new BasicBSONObject();
                    obj.put( "name", name + i );
                    obj.put( "age", age );
                    expectDataList.add( obj );
                }
            }
            ArrayList< BSONObject > actualDataList = new ArrayList< BSONObject >();
            String selectData = "select * from " + csName + "." + clName;
            DBCursor allRecords = sdb.exec( selectData );
            while ( allRecords.hasNext() ) {
                BSONObject record = allRecords.getNext();
                record.removeField( "_id" );
                actualDataList.add( record );
            }
            Assert.assertEquals( actualDataList, expectDataList );
        } catch ( BaseException e ) {
            Assert.fail(
                    "insert or select data failed, errMsg:" + e.getMessage() );
        }

        // drop cs and cl
        try {
            sdb.exec( "drop collection " + csName + "." + clName );
            DBCursor clCursor = sdb.exec( "list collections" );
            while ( clCursor.hasNext() ) {
                String actualCLName = ( String ) clCursor.getNext()
                        .get( "Name" );
                if ( actualCLName.equals( clName ) ) {
                    Assert.fail( "drop cl failed" );
                }
            }
            sdb.exec( "drop collectionspace " + csName );
            DBCursor csCursor = sdb.exec( "list collectionspaces" );
            while ( csCursor.hasNext() ) {
                String actualCSName = ( String ) csCursor.getNext()
                        .get( "Name" );
                if ( actualCSName.equals( csName ) ) {
                    Assert.fail( "drop cs failed" );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "drop cs and cl failed, errMsg:" + e.getMessage() );
        }

        // clear env
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
    }
}
