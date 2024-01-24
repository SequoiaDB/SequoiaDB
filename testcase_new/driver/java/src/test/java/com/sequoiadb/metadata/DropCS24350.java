package com.sequoiadb.metadata;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-24350:删除CS指定Option参数
 * @Author YiPan
 * @CreateDate 2021/9/11
 * @Version 1.0
 */
public class DropCS24350 extends SdbTestBase {
    private Sequoiadb sdb;
    private String csName = "cs24350";
    private String clName = "cl24350";
    private final String EnsureEmpty = "EnsureEmpty";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() {
        // cs下存在cl，指定option中EnsureEmpty=false
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
        sdb.dropCollectionSpace( csName,
                new BasicBSONObject( EnsureEmpty, false ) );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );

        // b：cs下存在cl，不指定option
        cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
        sdb.dropCollectionSpace( csName );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );

        // c：cs下不存在cl，指定option中EnsureEmpty=true
        cs = sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName,
                new BasicBSONObject( EnsureEmpty, true ) );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );

        // d：cs下存在cl，指定option中EnsureEmpty=true
        cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
        try {
            sdb.dropCollectionSpace( csName,
                    new BasicBSONObject( EnsureEmpty, true ) );
            Assert.fail( "except fail but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorType() != SDBError.SDB_DMS_CS_NOT_EMPTY
                    .getErrorType() ) {
                throw e;
            }
        }
        Assert.assertTrue( sdb.isCollectionSpaceExist( csName ) );
    }

    @AfterClass
    public void tearDown() {
        sdb.dropCollectionSpace( csName );
        sdb.close();
    }

}