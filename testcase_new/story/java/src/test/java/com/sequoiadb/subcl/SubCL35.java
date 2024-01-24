package com.sequoiadb.subcl;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;

import org.testng.annotations.BeforeClass;

import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;

import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestName:SEQDB-35 attach子表时，左右区间范围类型不同 :1.创建主表，子表；
 *                    2.挂载子表时，指定左区间的Key为String类型，右区间的key为int类型；
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL35 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL35";
    private String subCLName = "subCL35";

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( coordUrl, "", "" );
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException( "StandAlone skip this test:"
                        + this.getClass().getName() );
            }
            commCS = sdb.getCollectionSpace( csName );
            mainCL = commCS.createCollection( mainCLName, ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{\"alph\":-1}}" ) );
            subCL = commCS.createCollection( subCLName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"tx_id\":1},ShardingType:\"hash\"}" ) );
        } catch ( BaseException e ) {
            Assert.fail( "TestCase35 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // 挂载子表时，指定左区间的Key为String类型，右区间的key为int类型
    @Test
    public void test() {
        DBCursor dbc = null;
        try {
            mainCL.attachCollection( this.subCL.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":\"boy\"},UpBound:{\"alph\":9}}" ) );
            BSONObject bobj = new BasicBSONObject();
            bobj.put( "alph", "abc" );
            bobj.put( "age", 89 );
            bobj.put( "name", "Tom" );
            mainCL.insert( bobj );

            // 比对结果
            if ( !SubCLUtils2.isCollectionContainThisJSON( subCL,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
            if ( !SubCLUtils2.isCollectionContainThisJSON( mainCL,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( subCLName );
            commCS.dropCollection( mainCLName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }
}
