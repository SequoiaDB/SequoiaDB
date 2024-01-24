package com.sequoiadb.meta;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestAlterCS14996 test content:Concurrent alter cs and create cl
 * testlink case:seqDB-14996
 * 
 * @author wuyan
 * @Date 2018.5.2
 * @version 1.00
 */
public class TestAlterCS14996 extends SdbTestBase {
    private String csName = "altercs14996";
    private String clName = "test14996";
    private static Sequoiadb sdb = null;
    private int existCLNums = 0;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName );
    }

    @Test
    public void testAlterCl() {
        int pagesize = 16384;
        AlterCS altercs = new AlterCS( pagesize );
        CreateCL createcl = new CreateCL();

        altercs.start();
        createcl.start();

        if ( altercs.isSuccess() ) {
            if ( !createcl.isSuccess() ) {
                Assert.assertTrue( !createcl.isSuccess(),
                        createcl.getErrorMsg() );
                BaseException e = ( BaseException ) ( createcl.getExceptions()
                        .get( 0 ) );
                if ( e.getErrorCode() != -6 && e.getErrorCode() != -32 ) {
                    Assert.fail( "create fail:" + createcl.getErrorMsg()
                            + "  e:" + e.getErrorCode() );
                }
                checkAlterCSResult( pagesize );
                checkCreateCLResult( false );

            } else {
                // and maybe all operations are sucessful,
                Assert.assertTrue( createcl.isSuccess() );
                checkAlterCSResult( pagesize );
                checkCreateCLResult( true );
            }
        } else if ( !altercs.isSuccess() ) {
            BaseException e = ( BaseException ) ( altercs.getExceptions()
                    .get( 0 ) );
            if ( e.getErrorCode() != -32 && e.getErrorCode() != -275 ) {
                Assert.fail( "altercs fail:" + altercs.getErrorMsg() + "  e:"
                        + e.getErrorCode() );
            }
            Assert.assertTrue( createcl.isSuccess(), createcl.getErrorMsg() );
            checkCreateCLResult( true );
            int expPageSize = 65536;
            checkAlterCSResult( expPageSize );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class AlterCS extends SdbThreadBase {
        private int pageSize;

        public AlterCS( int pageSize ) {
            this.pageSize = pageSize;
        }

        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{PageSize:" + pageSize + "}" );
                dbcs.setAttributes( options );
            }
        }
    }

    private class CreateCL extends SdbThreadBase {
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                for ( int i = 0; i < 10; i++ ) {
                    dbcs.createCollection( clName + "." + i );
                    existCLNums = i;
                }

            }
        }
    }

    private void checkAlterCSResult( int expPageSize ) {
        Node nodeConnect = sdb.getReplicaGroup( "SYSCatalogGroup" ).getMaster();
        try ( Sequoiadb dbca = nodeConnect.connect()) {
            String matcher = "{'Name':'" + csName + "'}";
            DBCollection dbcl = dbca.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONSPACES" );
            DBCursor cursor = dbcl.query( matcher, null, null, null );
            BasicBSONObject actRecs = null;
            while ( cursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) cursor.getNext();
                int pageSize = ( int ) actRecs.get( "PageSize" );
                Assert.assertEquals( pageSize, expPageSize );
            }
            cursor.close();
        }
    }

    private void checkCreateCLResult( boolean isSuccessCreateCL ) {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( isSuccessCreateCL ) {
            for ( int i = 0; i < 10; i++ ) {
                Assert.assertEquals( cs.isCollectionExist( clName + "." + i ),
                        true, clName + "." + i + " is not exist!" );
            }
        } else {
            int count = 0;
            for ( int i = 0; i < 10; i++ ) {
                if ( cs.isCollectionExist( clName + "." + i ) ) {
                    count++;
                }
                Assert.assertEquals( count, existCLNums );
                if ( count == 10 ) {
                    Assert.fail( "create cl should be fail!" );
                }
            }
        }
    }

}
