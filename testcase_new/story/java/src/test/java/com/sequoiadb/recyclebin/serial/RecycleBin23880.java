package com.sequoiadb.recyclebin.serial;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23880:并发修改回收站属性
 * @Author liuli
 * @Date 2021.07.02
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin23880 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private List< BasicBSONObject > attrs = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test
    public void test() throws Exception {
        attrs.add( new BasicBSONObject( "ExpireTime", 5600 ) );
        attrs.add( new BasicBSONObject( "ExpireTime", 8600 ) );
        attrs.add( new BasicBSONObject( "ExpireTime", 10000 ) );
        attrs.add( new BasicBSONObject( "MaxItemNum", 1500 ) );
        attrs.add( new BasicBSONObject( "MaxItemNum", 2000 ) );
        attrs.add( new BasicBSONObject( "MaxItemNum", 2500 ) );
        attrs.add( new BasicBSONObject( "AutoDrop", false ) );
        attrs.add( new BasicBSONObject( "AutoDrop", true ) );
        // 修改回收站属性
        ThreadExecutor es = new ThreadExecutor();
        for ( BasicBSONObject attr : attrs ) {
            es.addWorker( new AlterRecycleBin( attr ) );
        }
        es.run();

        BSONObject attributes = sdb.getRecycleBin().getDetail();
        int expireTime = ( int ) attributes.get( "ExpireTime" );
        int maxItemNum = ( int ) attributes.get( "MaxItemNum" );
        boolean autoDrop = ( boolean ) attributes.get( "AutoDrop" );
        if ( expireTime != 5600 && expireTime != 8600 && expireTime != 10000 ) {
            Assert.fail( "Incorrect recycle bin properties, expire time is "
                    + expireTime );
        }
        if ( maxItemNum != 1500 && maxItemNum != 2000 && maxItemNum != 2500 ) {
            Assert.fail( "Incorrect recycle bin properties, max item num is "
                    + maxItemNum );
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.getRecycleBin().alter( new BasicBSONObject( "ExpireTime", 4320 ) );
        sdb.getRecycleBin().alter( new BasicBSONObject( "MaxItemNum", 1000 ) );
        sdb.getRecycleBin().alter( new BasicBSONObject( "AutoDrop", false ) );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private static class AlterRecycleBin {
        private BasicBSONObject option;

        private AlterRecycleBin( BasicBSONObject option ) {
            this.option = option;
        }

        @ExecuteOrder(step = 1)
        private void alterRecycleBin() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().alter( option );
            }
        }
    }

}
