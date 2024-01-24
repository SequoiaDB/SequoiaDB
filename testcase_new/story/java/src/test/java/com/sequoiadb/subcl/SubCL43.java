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
 * @FileName:SEQDB-43 attach子表时，指定无穷大与无穷小 :1.创建主表，子表；
 *                    2.attach子表使用下限$minkey和上限$maxkey；[需要在信息中心查找一下]
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL43 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL_1;
    private String mainCLName = "mainCL43";
    private String subCLName_1 = "subCL43_1";

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
        } catch ( BaseException e ) {
            Assert.fail( "TestCase43 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
    }

    // attach子表使用下限$minkey和上限$maxkey
    @Test
    public void test() {
        try {
            this.mainCL.attachCollection( this.subCL_1.getFullName(),
                    ( BSONObject ) JSON.parse(
                            "{LowBound:{\"alph\":{$minKey:1}},UpBound:{\"alph\":{$maxKey:1}}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( subCLName_1 );
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
