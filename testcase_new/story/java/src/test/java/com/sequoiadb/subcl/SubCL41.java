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
 * @TestName:SEQDB-41 attach不指定主表的ShardingKey为区间范围字段 :1.创建主表,shardingkey:time；
 *                    2.创建子表； 3.attach子表时区间的key不为time；
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL41 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL41";
    private String subCLName = "subCL41";

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
                    .parse( "{IsMainCL:true,ShardingKey:{\"time\":1}}" ) );
            subCL = commCS.createCollection( subCLName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"tx_id\":1},ShardingType:\"hash\"}" ) );
        } catch ( BaseException e ) {
            Assert.fail( "TestCase41 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // 主表shardingkey:time,attach子表时区间的key不为time
    @Test
    public void test() {
        try {
            mainCL.attachCollection( this.subCL.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":100},UpBound:{\"alph\":200}}" ) );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -238, e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
            return;
        }
        Assert.fail( this.getClass().getName()
                + " dose not pass, mainCL(shardingkey:{time:1})"
                + " attach subCL {LowBound:{\"alph\":100},UpBound:{\"alph\":200}} success" );
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
