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
 * @FileName:SEQDB-44 attach子表时，指定无穷大与无穷小 :1.创建主表，子表； 2.attach子表时，只写了CL，没写为CS.CL
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL44 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL44";
    private String subCLName = "subCL44";

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
            Assert.fail( "TestCase44 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
    }

    // attach子表时，只写了CL，没写为CS.CL
    @Test
    public void test() {
        try {
            mainCL.attachCollection( subCL.getName(), ( BSONObject ) JSON.parse(
                    "{LowBound:{\"alph\":100},UpBound:{\"alph\":200}}" ) );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
            return;
        }
        Assert.fail();

    }

    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( mainCLName );
            commCS.dropCollection( subCLName );
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
