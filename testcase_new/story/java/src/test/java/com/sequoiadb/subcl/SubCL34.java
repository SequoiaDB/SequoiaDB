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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;

import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestName:SEQDB-34 attach子表基本功能验证 :1.创建主表，子表； 2.挂载子表时指定区间的Key为String类型；
 *                    3.往子表内写入区间同类型的属于区间的数据； 4.验证数据写入是否成功和正确；
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL34 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL34";
    private String subCLName = "subCL34";

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
                    .parse( "{IsMainCL:true,ShardingKey:{\"alph\":1}}" ) );
            subCL = commCS.createCollection( subCLName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"tx_id\":1},ShardingType:\"hash\"}" ) );
        } catch ( BaseException e ) {
            Assert.fail( "TestCase34 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // 挂载子表时指定区间的Key为String类型,往子表内写入区间同类型的属于区间的数据,验证数据写入是否成功和正确；
    @Test
    public void test() {
        try {
            mainCL.attachCollection( subCL.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{\"alph\":\"boy\"},UpBound:{\"alph\":\"girl\"}}" ) );
            BSONObject bobj = new BasicBSONObject();
            bobj.put( "alph", "boy" );
            bobj.put( "age", 89 );
            bobj.put( "name", "Tom" );
            BSONObject bobj1 = new BasicBSONObject();
            bobj1.put( "alph", "cat" );
            bobj1.put( "age", 100 );
            bobj1.put( "name", "jery" );
            mainCL.insert( bobj );
            mainCL.insert( bobj1 );

            // 检查是否插入成功
            if ( !SubCLUtils2.isCollectionContainThisJSON( subCL,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
            if ( !SubCLUtils2.isCollectionContainThisJSON( subCL,
                    bobj1.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
            if ( !SubCLUtils2.isCollectionContainThisJSON( mainCL,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
            if ( !SubCLUtils2.isCollectionContainThisJSON( mainCL,
                    bobj1.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
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
