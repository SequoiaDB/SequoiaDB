package com.sequoiadb.baseexception;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:BaseExceptionTest16536 验证CollectionSpace类下接口引擎报错
 * @author wangkexin
 * @Date 2018-11-21
 * @version 1.00
 */

public class BaseExceptionTest16536 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "notexistcl16536";
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
    }

    @Test
    public void test() {
        // getCollection
        try {
            this.cs.getCollection( clName );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_DMS_NOTEXIST.getErrorCode(),
                    e.getErrorCode() );
            Assert.assertEquals(
                    "{ \"errno\" : " + SDBError.SDB_DMS_NOTEXIST.getErrorCode()
                            + " , \"description\" : \""
                            + SDBError.SDB_DMS_NOTEXIST.getErrorDescription()
                            + "\" , \"detail\" : \"\" }",
                    e.getErrorObject().toString() );
        }

        // dropCollection
        try {
            this.cs.dropCollection( clName );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            BSONObject errObject = e.getErrorObject();
            Assert.assertEquals( errObject.get( "errno" ),
                    SDBError.SDB_DMS_NOTEXIST.getErrorCode() );
            Assert.assertEquals( errObject.get( "description" ).toString(),
                    SDBError.SDB_DMS_NOTEXIST.getErrorDescription() );
            ReplicaGroup rg = this.sdb.getReplicaGroup( "SYSCatalogGroup" );
            String expected = "[{ \"NodeName\" : \""
                    + rg.getMaster().getNodeName()
                    + "\" , \"GroupName\" : \"SYSCatalogGroup\" , \"Flag\" : "
                    + SDBError.SDB_DMS_NOTEXIST.getErrorCode()
                    + " , \"ErrInfo\" : { \"errno\" : "
                    + SDBError.SDB_DMS_NOTEXIST.getErrorCode()
                    + " , \"description\" : \""
                    + SDBError.SDB_DMS_NOTEXIST.getErrorDescription()
                    + "\" , \"detail\" : \"\" } }]";
            Assert.assertEquals( errObject.get( "ErrNodes" ).toString(),
                    expected );
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.close();
    }
}
