package com.sequoiadb.subcl;

import java.util.Date;

import org.bson.BSONObject;
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
 * @TestName:SEQDB-37 attach子表时，区间范围类型为所有的字段类型 :1.创建主表，子表1,子表2；
 *                    2.attach子表1和子表2时的区间有重叠；
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL37 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL_1;
    private DBCollection subCL_2;
    private String mainCLName = "mainCL37";
    private String subCLName_1 = "subCL37_1";
    private String subCLName_2 = "subCL37_2";

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
            Assert.fail( "TestCase37 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // attach子表1和子表2时的区间有重叠
    @Test
    public void test() {
        try {
            this.mainCL.attachCollection( this.subCL_1.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":10},UpBound:{\"alph\":100}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
        try {
            this.mainCL.attachCollection( this.subCL_2.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":20},UpBound:{\"alph\":110}}" ) );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -237, e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
            return;
        }
        Assert.fail(
                "TestCase37 dose not pass, mainCL attach subCL_1{LowBound:{\"alph\":10},UpBound:{\"alph\":100}} and "
                        + "subCL_2{LowBound:{\"alph\":10},UpBound:{\"alph\":100}} success" );

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
