package com.sequoiadb.datasrc;

import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-22885:使用数据源创建cl，执行lob操作(随机读写lob)
 * @author wuyan
 * @Date 2021.1.9
 * @version 1.10
 */

public class DataSource22885 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "jdatasource22885";
    private String srcCSName = "jcssrc_22885";
    private String csName = "jcs_22885";
    private String clName = "jcl_22885";
    private DBCollection dbcl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + clName );
        dbcl = cs.createCollection( clName, options );
    }

    @Test
    public void test() throws Exception {
        int lobSize = 1024 * 1024;
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );
        // seek and write lob
        int newDataSize = 1024 * 400;
        int offset = 1024 * 500;
        byte[] rewriteBuff = RandomWriteLobUtil.getRandomBytes( newDataSize );
        try ( DBLob lob = dbcl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( rewriteBuff );
        }

        // read lob and check the lob content
        byte[] expBuff = RandomWriteLobUtil.appendBuff( lobBuff, rewriteBuff,
                offset );
        try ( DBLob lob = dbcl.openLob( oid )) {
            byte[] actualBuff = new byte[ ( int ) lob.getSize() ];
            lob.read( actualBuff );
            RandomWriteLobUtil.assertByteArrayEqual( actualBuff, expBuff );
        }

        // lockandseek，than rewrite lob
        int readLobSize = 1024 * 400;
        byte[] rewriteBuff1 = RandomWriteLobUtil.getRandomBytes( readLobSize );
        try ( DBLob lob = dbcl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lockAndSeek( offset, rewriteBuff1.length );
            lob.write( rewriteBuff1 );
        }
        RandomWriteLobUtil.checkRewriteLobResult( dbcl, oid, offset,
                rewriteBuff1, expBuff );
    }

    @AfterClass
    public void tearDown() {
        try {
            DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
            srcdb.dropCollectionSpace( srcCSName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }
}
