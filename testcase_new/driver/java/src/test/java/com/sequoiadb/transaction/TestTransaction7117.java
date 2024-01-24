package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-7117:ִ������������ع�����
 * @Author chensiqin
 * @Date 2016-09-19
 */

public class TestTransaction7117 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7117";
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        String commCSName = SdbTestBase.csName;
        try {
            this.sdb = new Sequoiadb( coordAddr, "", "" );
            if ( !this.sdb.isCollectionSpaceExist( commCSName ) ) {
                try {
                    this.cs = this.sdb.createCollectionSpace( commCSName );
                } catch ( BaseException e ) {
                    Assert.assertEquals( -33, e.getErrorCode(),
                            e.getMessage() );
                }
            } else {
                this.cs = this.sdb.getCollectionSpace( commCSName );
            }
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.cl = this.cs.createCollection( clName );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestTransaction7117 setUp error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestTransaction7117 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    public void testTransactionRollBack() {
        if ( !Util.isCluster( this.sdb ) ) {
            return;
        }
        try {
            this.sdb.beginTransaction();
            insertData();
            DBCursor cursor = this.cl.query();
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                actualList.add( cursor.getNext() );
            }
            cursor.close();

            // before rollback check insert
            Assert.assertEquals( actualList, this.insertRecods );
            DBQuery dbQuery = new DBQuery();
            dbQuery.setModifier(
                    ( BSONObject ) JSON.parse( "{$set:{num:22}}" ) );
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            this.cl.update( dbQuery );
            cursor = this.cl.query();
            actualList.clear();
            while ( cursor.hasNext() ) {
                actualList.add( cursor.getNext() );
            }
            cursor.close();
            for ( int i = 0; i < this.insertRecods.size(); i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj = this.insertRecods.get( i );
                obj.put( "num", 22 );
                expectedList.add( obj );
            }
            Assert.assertEquals( actualList, expectedList );// before rollback
                                                            // check update
            this.cl.delete( ( BSONObject ) JSON.parse( "{_id:{$et:0}}" ) );
            cursor = this.cl.query();
            actualList.clear();
            while ( cursor.hasNext() ) {
                actualList.add( cursor.getNext() );
            }
            cursor.close();
            expectedList.clear();
            for ( int i = 1; i < this.insertRecods.size(); i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj = this.insertRecods.get( i );
                expectedList.add( obj );
            }
            Assert.assertEquals( actualList, expectedList );// before rollback
                                                            // check delete

            this.sdb.rollback();

            cursor = this.cl.query();
            actualList.clear();
            while ( cursor.hasNext() ) {
                actualList.add( cursor.getNext() );
            }
            cursor.close();
            expectedList.clear();
            Assert.assertEquals( actualList, expectedList );// check rollback
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestTransaction7117 testTransactionRollBack error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestTransaction7117 testTransactionRollBack error, error description:"
                            + e.getMessage() );
        }
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            for ( int i = 0; i < 5; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "name", "zhangsan" + i );
                bson.put( "num", i );
                this.insertRecods.add( bson );
            }
            this.cl.insert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestTransaction7116 insertData error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestTransaction7116 insertData error, error description:"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.sdb.close();
    }
}
