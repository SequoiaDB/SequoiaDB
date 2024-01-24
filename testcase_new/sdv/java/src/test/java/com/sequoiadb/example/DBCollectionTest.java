package com.sequoiadb.example;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class DBCollectionTest extends SdbTestBase {
    // private static String coordUrl = "192.168.30.78:11810/t.t";
    // private String csName = null;
    private String clName = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void connectSdb() {
        /*
         * String []tmp = coordUrl.split("/"); if (2 != tmp.length){ System.out.
         * printf("input coordUrl: %s, request [host:port/csprefix.clprefix]");
         * Assert.assertTrue(false); } String connStr = tmp[0]; String[] names =
         * tmp[1].split("\\."); if (2 == names.length){ csName = names[0] +
         * Thread.currentThread().getId(); clName = names[1] +
         * Thread.currentThread().getId(); }else{ csName = names[0] +
         * Thread.currentThread().getId(); clName = names[0] +
         * Thread.currentThread().getId(); }
         */

        if ( SdbTestBase.csName == null ) {
            Assert.fail( "csName is null" );
        }

        clName = "java_basic_oper" + Thread.currentThread().getId();
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            System.out.printf( "connect %s failed, errMsg:%s\n", coordUrl,
                    e.getMessage() );
            Assert.assertTrue( false );
        }

        try {
            if ( sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                cs = sdb.getCollectionSpace( SdbTestBase.csName );
            } else {
                Assert.fail( SdbTestBase.csName + " not exist" );
            }
        } catch ( BaseException e ) {
            System.out.printf(
                    "get|crate CollectionSpace %s failed, errMsg:%s\n",
                    SdbTestBase.csName, e.getMessage() );
            Assert.assertTrue( false );
        }

        try {
            if ( cs.isCollectionExist( clName ) ) {
                cl = cs.getCollection( clName );
            } else {
                cl = cs.createCollection( clName,
                        ( BSONObject ) JSON.parse( "{ReplSize:0}" ) );
            }
        } catch ( BaseException e ) {
            System.out.printf(
                    "get|crate CollectionSpace %s failed, errMsg:%s\n",
                    SdbTestBase.csName + "." + clName, e.getMessage() );
            Assert.assertTrue( false );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void releaseSdb() {
        /*
         * try{ sdb.dropCollectionSpace(csName); }catch(BaseException e){
         * System.out.printf("drop CollectionSpace %s failed, errMsg:%s\n",
         * csName + "." + clName, e.getMessage()); Assert.assertTrue(false); }
         */

        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            System.out.printf( "disconnect failed, errMsg:%s\n",
                    SdbTestBase.csName + "." + clName, e.getMessage() );
            Assert.assertTrue( false );
        }
    }

    @BeforeMethod
    public void loadDoc() {

    }

    @AfterMethod
    public void removeAllDoc() {
        try {
            BSONObject cond = new BasicBSONObject();
            cl.delete( cond );
        } catch ( BaseException e ) {
            System.out.printf( "remove() failed, errMsg:%s\n", e.getMessage() );
            Assert.assertTrue( false );
        }
    }

    private BSONObject getRuleDoc( BSONObject doc ) {
        BSONObject rule = new BasicBSONObject();
        BSONObject subDoc = new BasicBSONObject();
        Object obj = doc.get( "random" );
        if ( obj instanceof Integer ) {
            subDoc.put( "random", 1 );
            rule.put( "$inc", subDoc );
        } else if ( obj instanceof Float ) {
            subDoc.put( "random", 1.0f );
            rule.put( "$set", subDoc );
        } else if ( obj instanceof String ) {
            subDoc.put( "random", "" );
            rule.put( "$unset", subDoc );
        } else if ( obj instanceof Boolean ) {
            BasicBSONList arr = new BasicBSONList();
            arr.put( "0", 0 );
            arr.put( "1", 1 );
            subDoc.put( "addField", arr );
            rule.put( "$addtoset", subDoc );
        } else {
            subDoc.put( "random", "random" );
            rule.put( "$replace", subDoc );
        }
        return rule;
    }

    @Test(dataProvider = "create", dataProviderClass = DocProvider.class)
    public void testBasicOper( BSONObject doc ) {
        try {
            cl.insert( doc );
        } catch ( BaseException e ) {
            // System.out.printf("insert(%s) failed,
            // errMsg:%s\n",doc.toString(),e.getMessage());
            Assert.assertTrue( false, e.getMessage() );
        }

        // System.out.println("insert doc=" + doc.toString());
        boolean findDoc = false;
        BSONObject cond = null;
        try {
            cond = new BasicBSONObject();
            cond.put( "_id", doc.get( "_id" ) );
            DBCursor cr = cl.query( cond, null, null, null );
            while ( cr.hasNext() ) {
                BSONObject obj = cr.getNext();
                // System.out.println("query obj=" + obj.toString());
                findDoc = true;
                if ( obj.containsField( "typebinary" )
                        || obj.containsField( "typeobj" )
                        || obj.containsField( "typearr" ) ) {
                    continue;
                }
                Assert.assertTrue( obj.equals( doc ),
                        obj.toString() + "doc:" + doc.toString() );

            }
        } catch ( BaseException e ) {
            System.out.printf( "query(%s) failed, errMsg:%s\n", cond.toString(),
                    e.getMessage() );
            Assert.assertTrue( false, e.getMessage() );
        }

        Assert.assertEquals( findDoc, true );
        BSONObject rule = null;
        try {
            rule = getRuleDoc( doc );
            cl.update( cond, rule, null );
        } catch ( BaseException e ) {
            System.out.printf( "update(%s, %s) failed, errMsg:%s\n",
                    cond.toString(), rule.toString(), e.getMessage() );
            Assert.assertTrue( false, e.getMessage() );
        }

        try {
            cond = new BasicBSONObject();
            cond.put( "_id", doc.get( "_id" ) );
            DBCursor cr = cl.query( cond, null, null, null );
            while ( cr.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) cr.getNext();
                Object subobj = doc.get( "random" );
                if ( subobj instanceof Integer ) {
                    Assert.assertEquals( ( Integer ) subobj + 1,
                            obj.getInt( "random" ) );
                } else if ( subobj instanceof Float ) {
                    Assert.assertEquals( 1.0, obj.getDouble( "random" ) );
                } else if ( subobj instanceof String ) {
                    Assert.assertEquals( null, obj.getString( "random" ) );
                } else if ( subobj instanceof Boolean ) {
                    BasicBSONList arr = new BasicBSONList();
                    arr.put( "0", 0 );
                    arr.put( "1", 1 );
                    Assert.assertTrue( obj.get( "addField" ).equals( arr ),
                            arr.toString() );
                } else {
                    Assert.assertEquals( "random", obj.getString( "random" ) );
                }
                findDoc = true;
            }
        } catch ( BaseException e ) {
            System.out.printf( "query(%s) failed, errMsg:%s\n", cond.toString(),
                    e.getMessage() );
            Assert.assertTrue( false, e.getMessage() );
        }

        try {
            cl.delete( cond );
        } catch ( BaseException e ) {
            System.out.printf( "delete(%s) failed, errMsg:%s\n",
                    cond.toString(), e.getMessage() );
            Assert.assertTrue( false, e.getMessage() );
        }

        findDoc = false;
        try {
            DBCursor cr = cl.query( cond, null, null, null );
            while ( cr.hasNext() ) {
                findDoc = true;
            }
        } catch ( BaseException e ) {
            System.out.printf( "query(%s) failed, errMsg:%s\n", cond.toString(),
                    e.getMessage() );
            Assert.assertTrue( false, e.getMessage() );
        }
        Assert.assertEquals( findDoc, false );
    }
}
