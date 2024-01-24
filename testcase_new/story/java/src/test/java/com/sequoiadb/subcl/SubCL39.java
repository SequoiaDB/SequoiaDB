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
 * @TestName:SEQDB-39 attach功能异常操作验证 :1.创建主表； 2.attach一个未创建的表
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL39 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private String mainCLName = "mainCL39";
    private String subCLName = "subCL39";

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
        } catch ( BaseException e ) {
            Assert.fail( "TestCase39 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // attach一个未创建的表
    @Test
    public void test() {
        try {
            mainCL.attachCollection( commCS.getName() + "." + subCLName,
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":110},UpBound:{\"alph\":210}}" ) );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
            return;
        }
        Assert.fail( this.getClass().getName()
                + " dose not pass, mainCL attach a non-existing subCL success" );
    }

    @AfterClass
    public void tearDown() {

        try {
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
