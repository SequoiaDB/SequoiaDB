package com.sequoiadb.lob.randomwrite;

import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13239:lock the data segment to write lob,the lock offset
 *              exceeds the maximum length of lob seqDB-18971
 *              主子表锁定偏移超过lob文件大小，写入lob
 * @author wuyan
 * @Date 2017.11.7
 * @version 1.00
 */
public class RewriteLob13239_18971 extends SdbTestBase {
    private String clName = "writelob13239";
    private String mainCLName = "maincl18971";
    private String subCLName = "subcl18971";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private Random random = new Random();

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13239
                new Object[] { clName },
                // testcase:18971
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash'}";
        RandomWriteLobUtil.createCL( cs, clName, clOptions );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        int writeSize = random.nextInt( 1024 * 1024 * 2 );
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, lobBuff );

        int offset = writeSize + random.nextInt( 1024 * 254 );
        int rewriteLobSize = random.nextInt( 1024 * 1024 * 2 );
        byte[] rewriteBuff = RandomWriteLobUtil
                .getRandomBytes( rewriteLobSize );
        rewriteLob( cl, oid, offset, rewriteBuff );
        // check the rewrite lob
        byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( cl, oid,
                rewriteBuff.length, offset );
        RandomWriteLobUtil.assertByteArrayEqual( actBuff, rewriteBuff );
        // check the old lob
        byte[] actOldLobBuff = RandomWriteLobUtil.seekAndReadLob( cl, oid,
                lobBuff.length, 0 );
        RandomWriteLobUtil.assertByteArrayEqual( actOldLobBuff, lobBuff );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void rewriteLob( DBCollection cl, ObjectId oid, int offset,
            byte[] rewriteBuff ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lockAndSeek( offset, rewriteBuff.length );
            lob.write( rewriteBuff );
        }
    }

}
