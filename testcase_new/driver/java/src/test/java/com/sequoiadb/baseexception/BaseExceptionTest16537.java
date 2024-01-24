package com.sequoiadb.baseexception;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:BaseExceptionTest16537 验证Sequoiadb类下接口引擎报错
 * @author wangkexin
 * @Date 2018-11-21
 * @version 1.00
 */

public class BaseExceptionTest16537 extends SdbTestBase {
    private Sequoiadb sdb;
    private String csName = "notexistcs16537";
    private String sameCsName = "cs16537";
    private String sameDoaminName = "domain16537";
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
    }

    @Test
    public void test() {
        // create duplicate collection space
        try {
            this.sdb.createCollectionSpace( sameCsName );
            this.sdb.createCollectionSpace( sameCsName );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            BSONObject errObject = e.getErrorObject();
            Assert.assertEquals( errObject.get( "errno" ),
                    SDBError.SDB_DMS_CS_EXIST.getErrorCode() );
            Assert.assertEquals( errObject.get( "description" ).toString(),
                    SDBError.SDB_DMS_CS_EXIST.getErrorDescription() );
            ReplicaGroup rg = this.sdb.getReplicaGroup( "SYSCatalogGroup" );
            String expected = "[{ \"NodeName\" : \""
                    + rg.getMaster().getNodeName()
                    + "\" , \"GroupName\" : \"SYSCatalogGroup\" , \"Flag\" : "
                    + SDBError.SDB_DMS_CS_EXIST.getErrorCode()
                    + " , \"ErrInfo\" : { \"errno\" : "
                    + SDBError.SDB_DMS_CS_EXIST.getErrorCode()
                    + " , \"description\" : \""
                    + SDBError.SDB_DMS_CS_EXIST.getErrorDescription()
                    + "\" , \"detail\" : \"\" } }]";
            Assert.assertEquals( errObject.get( "ErrNodes" ).toString(),
                    expected );
        }

        // drop not exist cs
        try {
            this.sdb.dropCollectionSpace( csName );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            BSONObject errObject = e.getErrorObject();
            Assert.assertEquals( errObject.get( "errno" ),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
            Assert.assertEquals( errObject.get( "description" ).toString(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorDescription() );
            ReplicaGroup rg = this.sdb.getReplicaGroup( "SYSCatalogGroup" );
            String expected = "[{ \"NodeName\" : \""
                    + rg.getMaster().getNodeName()
                    + "\" , \"GroupName\" : \"SYSCatalogGroup\" , \"Flag\" : "
                    + SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                    + " , \"ErrInfo\" : { \"errno\" : "
                    + SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                    + " , \"description\" : \""
                    + SDBError.SDB_DMS_CS_NOTEXIST.getErrorDescription()
                    + "\" , \"detail\" : \"\" } }]";
            Assert.assertEquals( errObject.get( "ErrNodes" ).toString(),
                    expected );
        }

        // load not exist cs
        try {
            this.sdb.loadCollectionSpace( csName, null );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode(),
                    e.getErrorCode() );
            Assert.assertEquals(
                    "{ \"errno\" : "
                            + SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                            + " , \"description\" : \""
                            + SDBError.SDB_DMS_CS_NOTEXIST.getErrorDescription()
                            + "\" , \"detail\" : \"\" }",
                    e.getErrorObject().toString() );
        }

        // unload not exist cs
        try {
            this.sdb.unloadCollectionSpace( csName, null );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode(),
                    e.getErrorCode() );
            Assert.assertEquals(
                    "{ \"errno\" : "
                            + SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                            + " , \"description\" : \""
                            + SDBError.SDB_DMS_CS_NOTEXIST.getErrorDescription()
                            + "\" , \"detail\" : \"\" }",
                    e.getErrorObject().toString() );
        }

        // get not exist cs
        try {
            this.sdb.getCollectionSpace( csName );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode(),
                    e.getErrorCode() );
            Assert.assertEquals(
                    "{ \"errno\" : "
                            + SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                            + " , \"description\" : \""
                            + SDBError.SDB_DMS_CS_NOTEXIST.getErrorDescription()
                            + "\" , \"detail\" : \"\" }",
                    e.getErrorObject().toString() );
        }

        // create existing domain
        try {
            this.sdb.createDomain( sameDoaminName, null );
            this.sdb.createDomain( sameDoaminName, null );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            BSONObject errObject = e.getErrorObject();
            Assert.assertEquals( errObject.get( "errno" ),
                    SDBError.SDB_CAT_DOMAIN_EXIST.getErrorCode() );
            Assert.assertEquals( errObject.get( "description" ).toString(),
                    SDBError.SDB_CAT_DOMAIN_EXIST.getErrorDescription() );
            ReplicaGroup rg = this.sdb.getReplicaGroup( "SYSCatalogGroup" );
            String expected = "[{ \"NodeName\" : \""
                    + rg.getMaster().getNodeName()
                    + "\" , \"GroupName\" : \"SYSCatalogGroup\" , \"Flag\" : "
                    + SDBError.SDB_CAT_DOMAIN_EXIST.getErrorCode()
                    + " , \"ErrInfo\" : { \"errno\" : "
                    + SDBError.SDB_CAT_DOMAIN_EXIST.getErrorCode()
                    + " , \"description\" : \""
                    + SDBError.SDB_CAT_DOMAIN_EXIST.getErrorDescription()
                    + "\" , \"detail\" : \"\" } }]";
            Assert.assertEquals( errObject.get( "ErrNodes" ).toString(),
                    expected );
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.dropCollectionSpace( sameCsName );
        sdb.dropDomain( sameDoaminName );
        sdb.close();
    }
}
