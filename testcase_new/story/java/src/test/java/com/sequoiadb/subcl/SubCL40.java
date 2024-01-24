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
 * @TestName:SEQDB-40 attach同一子表，指定不同范围区间 :1.创建主表，子表； 2.attach同一个子表，指定不同区间
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL40 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL40";
    private String subCLName = "subCL40";

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
            Assert.fail( "TestCase40 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
    }

    // attach同一个子表，指定不同区间
    @Test
    public void test() {
        try {
            mainCL.attachCollection( this.subCL.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":100},UpBound:{\"alph\":200}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
        try {
            mainCL.attachCollection( this.subCL.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":300},UpBound:{\"alph\":350}}" ) );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -235, e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
            return;
        }
        Assert.fail( this.getClass().getName()
                + " dose not pass,Duplicated attach collection success" );

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
