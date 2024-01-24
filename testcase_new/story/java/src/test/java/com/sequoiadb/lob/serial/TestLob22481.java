package com.sequoiadb.lob.serial;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-22481:创建 lob 时执行 sync
 * @author liyuanyue
 * @Date 2020.7.24
 */

public class TestLob22481 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "_cl22481";
    private DBCollection cl = null;
    private int EXECNUMBERS = 300;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
    }

    @Test
    public void testLob() {
        CreateLob createLob = new CreateLob();
        ExecSync execSync = new ExecSync();

        createLob.start();
        execSync.start();

        Assert.assertTrue( createLob.isSuccess(), createLob.getErrorMsg() );
        Assert.assertTrue( execSync.isSuccess(), execSync.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateLob extends SdbThreadBase {

        @Override
        public void exec() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < EXECNUMBERS; i++ ) {
                    DBCollection cl1 = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    DBCollection cl2 = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    DBLob lob1 = cl1.createLob();
                    DBLob lob2 = cl2.createLob();
                    lob1.close();
                    lob2.close();
                }
            } catch ( BaseException e ) {
                throw e;
            }
        }
    }

    private class ExecSync extends SdbThreadBase {

        @Override
        public void exec() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( int i = 0; i < EXECNUMBERS; i++ ) {
                    db.sync( ( BSONObject ) JSON.parse( "{ Block:true } " ) );
                }
            } catch ( BaseException e ) {
                throw e;
            }
        }
    }

}
