package com.sequoiadb.meta;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.Stack;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: CreateAndDropCL10945.java test content:concurrent creation and
 * deletion of different cl, all cl in the different dataGroup. testlink
 * case:seqDB-10945
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class CreateAndDropCL10945 extends SdbTestBase {

    private String csName = "cs10945";
    private String clName = "cl10945";
    private static Sequoiadb sdb = null;
    private Stack< String > preClNames = new Stack<>();
    private Random random = new Random();
    String clGroupName = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        // used to delete the cl
        createPreCL();
    }

    @Test(invocationCount = 10, threadPoolSize = 10)
    public void createAndDropCL10945() {
        CreateCLThread createCLThread = new CreateCLThread();
        DropCLThread dropCLThread = new DropCLThread();

        createCLThread.start();
        dropCLThread.start();

        if ( !( createCLThread.isSuccess() && dropCLThread.isSuccess() ) ) {
            List< Exception > exceptions = new ArrayList<>();
            exceptions.addAll( createCLThread.getExceptions() );
            exceptions.addAll( dropCLThread.getExceptions() );

            String errMsg = "";
            for ( int i = 0; i < exceptions.size(); i++ ) {
                exceptions.get( i ).printStackTrace();
                errMsg += exceptions.get( i ).getMessage() + "\n";
            }
            Assert.fail( errMsg );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

    class CreateCLThread extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            Sequoiadb db1 = null;
            CollectionSpace cs1 = null;
            DBCollection dbcl = null;

            try {
                db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cs1 = db1.getCollectionSpace( csName );
                String curClName = clName + "_" + Thread.currentThread().getName();
                dbcl = cs1.createCollection( curClName );
                checkCreateCl( dbcl );
            } catch ( BaseException e ) {
                Assert.assertTrue( false, e.getMessage() );
            } finally {
                if ( db1 != null ) {
                    db1.disconnect();
                }
            }
        }
    }

    class DropCLThread extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            Sequoiadb db2 = null;
            CollectionSpace cs2 = null;
            try {
                db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cs2 = db2.getCollectionSpace( csName );
                String delClName = preClNames.pop();
                cs2.dropCollection( delClName );
                Assert.assertFalse( cs2.isCollectionExist( delClName ) );
            } catch ( BaseException e ) {
                Assert.assertTrue( false, "drop cl fail " + e.getMessage() );
            } finally {
                if ( db2 != null ) {
                    db2.disconnect();
                }
            }
        }
    }

    private void createPreCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( csName ) ) {
                sdb.createCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getErrorCode() + e.getMessage() );
        }

        try {
            CollectionSpace cs = null;
            cs = sdb.getCollectionSpace( csName );
            for ( int i = 0; i < 10; i++ ) {
                String preClName = "pre" + clName + "_" + i;
                cs.createCollection( preClName );
                preClNames.push( preClName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    @SuppressWarnings("deprecation")
    public void checkCreateCl( DBCollection cl ) {
        try {
            List< BSONObject > list = new ArrayList<>();
            long num = 10;
            for ( long i = 0; i < num; i++ ) {
                BSONObject obj = new BasicBSONObject();
                BSONObject arr = new BasicBSONList();
                arr.put( "0", ( int ) ( Math.random() * 100 ) );
                arr.put( "1", "test" + i );
                arr.put( "2", 2.34 );
                obj.put( "arrtest", arr );
                obj.put( "int", random.nextInt( 10000 ) );
                list.add( obj );
            }
            cl.bulkInsert( list, DBCollection.FLG_INSERT_CONTONDUP );
            Assert.assertEquals( cl.getCount(), num );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "insert fail " + e.getMessage() );
        }
    }

}
