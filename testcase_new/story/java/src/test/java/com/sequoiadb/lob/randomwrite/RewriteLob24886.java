package com.sequoiadb.lob.randomwrite;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-24886 单表加锁续写lob 覆盖测试jira单号：SEQUOIADBMAINSTREAM-7834
 * @Author zhongziming
 * @CreateDate 2021/12/28
 * @UpdateUser zhongziming
 * @UpdateDate 2021/12/28
 * @version 1.00
 */
public class RewriteLob24886 extends SdbTestBase {

    private String clName = "writelob24886";
    private String firstStr = "Sequoia";
    private String addtionStr = "DB";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0}";
        RandomWriteLobUtil.createCL( cs, clName, clOptions );
    }

    @Test
    public void seekWriteWithDirectIO() {
        DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBLob lob = dbcl.createLob();
        ObjectId lobid = lob.getID();

        byte[] firstContent = firstStr.getBytes();
        lob.write( firstContent );
        lob.close();

        lob = dbcl.openLob( lobid, DBLob.SDB_LOB_WRITE );
        byte[] addtionContent = addtionStr.getBytes();
        lob.lock( firstContent.length, addtionContent.length );
        lob.seek( 0, DBLob.SDB_LOB_SEEK_END );
        lob.write( addtionContent, 0, addtionContent.length );
        lob.close();

        byte[] allContent = new byte[ firstContent.length
                + addtionContent.length ];
        lob = dbcl.openLob( lobid, DBLob.SDB_LOB_READ );
        lob.read( allContent );
        lob.close();

        Assert.assertEquals( firstStr + addtionStr, new String( allContent ) );
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
}
