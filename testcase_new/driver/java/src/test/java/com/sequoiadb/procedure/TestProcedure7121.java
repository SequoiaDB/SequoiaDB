package com.sequoiadb.procedure;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.Sequoiadb.SptReturnType;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestProcedure7121,7122 crtJSProcedure (String code);rmProcedure
 *                                  (String name); listProcedures (BSONObject
 *                                  condition)
 * @author chensiqin
 * @Date 2016-09-19
 * @version 1.00
 */

public class TestProcedure7121 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "cl7121";

    @BeforeTest
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
            this.cs.createCollection( clName );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestProcedure7121 setUp error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestProcedure7121 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    public void test() {
        if ( !Util.isCluster( this.sdb ) ) {
            return;
        }
        testJSProcedure();
        // testRMProcedure();
    }

    public void testJSProcedure() {
        String code = "function sum_7121(x, y){return x/y;}";
        try {
            DBCursor dbCursor = this.sdb.listProcedures(
                    ( BSONObject ) JSON.parse( "{\"name\":\"sum_7121\"}" ) );
            while ( dbCursor.hasNext() ) {
                this.sdb.rmProcedure( "sum_7121" );
                break;
            }
            this.sdb.crtJSProcedure( code );
            DBCursor cursor = this.sdb.listProcedures(
                    ( BSONObject ) JSON.parse( "{\"name\":\"sum_7121\"}" ) );
            BSONObject actual = new BasicBSONObject();
            String expected = "{ \"name\" : \"sum_7121\" , \"func\" : { \"$code\" : \"function sum_7121(x, y){return x/y;}\" } }";
            // expected.put("func", rcfunc);
            while ( cursor.hasNext() ) {
                actual = cursor.getNext();
                actual.removeField( "_id" );
                actual.removeField( "funcType" );
                System.out.println( actual.toString() );
                System.out.println( expected );
                Assert.assertEquals( actual.toString(), expected );
                // actual.removeField("func");
                // Assert.assertEquals(actual, expected);
                break;
            }
            cursor.close();
            this.sdb.rmProcedure( "sum_7121" );
            DBCursor cur = this.sdb.listProcedures(
                    ( BSONObject ) JSON.parse( "{\"name\":\"sum_7121\"}" ) );
            int count = 0;
            while ( cur.hasNext() ) {
                cur.getNext();
                count++;
            }
            cur.close();
            Assert.assertEquals( count, 0 );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestProcedure7121 testJSProcedure error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestProcedure7121 testJSProcedure error, error description:"
                            + e.getMessage() );
        }
    }

    /* test rmProcedure when the function name sum_7121 is not exist */
    public void testRMProcedure() {
        try {
            DBCursor cur = this.sdb.listProcedures(
                    ( BSONObject ) JSON.parse( "{\"name\":\"sum_7121\"}" ) );
            while ( cur.hasNext() ) {
                cur.getNext();
                this.sdb.rmProcedure( "sum_7121" );
            }
            cur.close();
            this.sdb.rmProcedure( "sum_7121" );
            Assert.fail( "Sequoiadb driver TestProcedure7121 expect error!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -233 );
        }
    }

    @Test
    public void testEvalWithWrongName() {
        if ( !Util.isCluster( this.sdb ) ) {
            return;
        }
        String code = "function sum_7121(x, y){return x / y ;}";
        try {
            DBCursor cur = this.sdb.listProcedures(
                    ( BSONObject ) JSON.parse( "{\"name\":\"sum_7121\"}" ) );
            while ( cur.hasNext() ) {
                cur.getNext();
                this.sdb.rmProcedure( "sum_7121" );
            }
            cur.close();
            this.sdb.crtJSProcedure( code );
            Sequoiadb.SptEvalResult evalResult2 = null;
            String code2 = "sum_7122(1,0.5)";
            evalResult2 = sdb.evalJS( code2 );
            DBCursor cursor = null;
            cursor = evalResult2.getCursor();
            Assert.assertNull( cursor,
                    "Sequoiadb driver TestProcedure7121 testEval no function sum_7121" );
            Sequoiadb.SptReturnType retType = null;
            retType = evalResult2.getReturnType();
            Assert.assertNull( retType,
                    "Sequoiadb driver TestProcedure7121 testEval no function sum_7121" );
            BSONObject errObj = null;
            errObj = evalResult2.getErrMsg();
            Assert.assertNotNull( errObj,
                    "Sequoiadb driver TestProcedure7121 testEval no function sum_7121" );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestProcedure7121 testEval error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestProcedure7121 testEval error, error description:"
                            + e.getMessage() );
        }
        try {
            sdb.rmProcedure( "sum_7121" );
        } catch ( BaseException e ) {
            System.out.println( "Failed to remove js procedure" );
            Assert.fail(
                    "Error message is: " + e.getMessage() + e.getErrorCode() );
        }
    }

    @Test
    public void testEvalJS() {
        if ( !Util.isCluster( this.sdb ) ) {
            return;
        }
        String code = "function sum_7121(x, y){ var z = x + y;}";
        String evalCode = "sum_7121(0.5, -2)";
        testEvalWithReturnType( code, evalCode,
                Sequoiadb.SptReturnType.TYPE_VOID );
        code = "function sum_7121(x, y){ return x*y;}";
        evalCode = "sum_7121(0.13,0.3)";
        testEvalWithReturnType( code, evalCode,
                Sequoiadb.SptReturnType.TYPE_NUMBER );
        code = "function sum_7121(x, y){ return x+y;}";
        evalCode = "sum_7121(\"Sequoia\",\"db\")";
        testEvalWithReturnType( code, evalCode,
                Sequoiadb.SptReturnType.TYPE_STR );
        code = "function sum_7121(x, y){ return true;}";
        evalCode = "sum_7121(1,2)";
        testEvalWithReturnType( code, evalCode,
                Sequoiadb.SptReturnType.TYPE_BOOL );
        code = "function sum_7121(x, y){ var o={a:123}; return o;}";
        evalCode = "sum_7121(1,2)";
        testEvalWithReturnType( code, evalCode,
                Sequoiadb.SptReturnType.TYPE_OBJ );
    }

    public void testEvalWithReturnType( String code, String evalCode,
            SptReturnType returnType ) {
        try {
            DBCursor cur = this.sdb.listProcedures(
                    ( BSONObject ) JSON.parse( "{\"name\":\"sum_7121\"}" ) );
            int count = 0;
            while ( cur.hasNext() ) {
                cur.getNext();
                count++;
            }
            cur.close();
            if ( count == 0 ) {
                this.sdb.crtJSProcedure( code );
            }
            Sequoiadb.SptEvalResult evalResult1 = null;
            evalResult1 = sdb.evalJS( evalCode );
            Assert.assertEquals( evalResult1.getReturnType(), returnType );
            Assert.assertEquals( evalResult1.getErrMsg(), null );
            DBCursor cursor = evalResult1.getCursor();
            BSONObject actual = new BasicBSONObject();
            BSONObject expected = new BasicBSONObject();
            while ( cursor.hasNext() ) {
                if ( cursor.getNext().containsField( "value" ) ) {
                    actual = cursor.getCurrent();
                }
            }
            cursor.close();
            if ( returnType.equals( Sequoiadb.SptReturnType.TYPE_NUMBER ) ) {
                expected.put( "value", 0.039 );
            }
            if ( returnType.equals( Sequoiadb.SptReturnType.TYPE_STR ) ) {
                expected.put( "value", "Sequoiadb" );
            }
            if ( returnType.equals( Sequoiadb.SptReturnType.TYPE_BOOL ) ) {
                expected.put( "value", true );
            }
            if ( returnType.equals( Sequoiadb.SptReturnType.TYPE_OBJ ) ) {
                String str = "{\"a\" : 123}";
                expected.put( "value", JSON.parse( str ) );
            }

            Assert.assertEquals( actual, expected );
        } catch ( BaseException e ) {
            System.out.println( "test testEvalWithReturnType" + returnType
                    + " error, error description:" + e.getMessage() );
            Assert.fail( "test testEvalWithReturnType" + returnType
                    + " error, error description:" + e.getMessage() );
        }

        try {
            sdb.rmProcedure( "sum_7121" );
        } catch ( BaseException e ) {
            System.out.println( "test testEvalWithReturnType" + returnType
                    + " failed to remove js procedure" );
            System.out.println( "test testEvalWithReturnType" + returnType
                    + " error message is: " + e.getMessage()
                    + e.getErrorCode() );
        }
    }

    @AfterTest
    public void tearDown() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.sdb.disconnect();
    }
}
