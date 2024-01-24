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
 * @TestName:SEQDB-38 attach子表的区间范围不连续，写入两个区间范围内的数据 :1.创建主表，两个子表；
 *                    2.attach的两个子表的区间范围没有相连； 3.写入数据验证；
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL38 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL_1;
    private DBCollection subCL_2;
    private String mainCLName = "mainCL38";
    private String subCLName_1 = "subCL38_1";
    private String subCLName_2 = "subCL38_2";

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
            subCL_1 = commCS.createCollection( subCLName_1, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"tx_id\":1},ShardingType:\"hash\"}" ) );
            subCL_2 = commCS.createCollection( subCLName_2, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"tx_id\":1},ShardingType:\"hash\"}" ) );
        } catch ( BaseException e ) {
            Assert.fail( "TestCase38 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // attach的两个子表的区间范围没有相连,写入数据验证
    @Test
    public void test() {
        try {
            this.mainCL.attachCollection( this.subCL_1.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":10},UpBound:{\"alph\":100}}" ) );
            this.mainCL.attachCollection( this.subCL_2.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":110},UpBound:{\"alph\":210}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
        BSONObject bobj = new BasicBSONObject();
        bobj.put( "alph", 80 );
        bobj.put( "age", 89 );
        bobj.put( "name", "Tom" );
        BSONObject bobj1 = new BasicBSONObject();
        bobj1.put( "alph", 120 );
        bobj1.put( "age", 89 );
        bobj1.put( "name", "Tom" );
        try {
            mainCL.insert( bobj );
            mainCL.insert( bobj1 );
            if ( !SubCLUtils2.isCollectionContainThisJSON( mainCL,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
            if ( !SubCLUtils2.isCollectionContainThisJSON( subCL_1,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
            if ( !SubCLUtils2.isCollectionContainThisJSON( subCL_2,
                    bobj1.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
        } catch ( Exception e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( subCLName_1 );
            commCS.dropCollection( subCLName_2 );
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
