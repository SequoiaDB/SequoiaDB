package com.sequoiadb.lob.randomwrite;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
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
 * @Description seqDB-13273:并发读lob，其中读数据范围有交集 seqDB-19015 主子表并发读lob，其中读数据范围有交集
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.11.22
 * @UpdateDate 2018.08.26
 * @version 1.10
 */
public class RewriteLob13273_19015 extends SdbTestBase {
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "lob_13273";
    private String mainCLName = "mainCL_19015";
    private String subCLName = "subCL_19015";

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clName
                // testcase:13273
                new Object[] { clName },
                // testcase:19015
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
        if ( !CommLib.isStandAlone( db ) ) {
            LobSubUtils.createMainCLAndAttachCL( db, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob13274( String clName ) throws InterruptedException {
        if ( CommLib.isStandAlone( db ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        int lobsize = 1024 * 1024 * 2;
        byte[] expectBytes = RandomWriteLobUtil.getRandomBytes( lobsize );
        DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBLob lob = dbcl.createLob();
        final ObjectId oid = lob.getID();
        lob.write( expectBytes );
        lob.close();

        List< DbLobReadTask > lobTasks = new ArrayList<>( 10 );

        int begin = 1024 * 10;
        int end = 1024 * 100;
        for ( int i = 1; i <= 10; i++ )
            lobTasks.add( new DbLobReadTask( SdbTestBase.csName, clName,
                    begin * i, end * i, oid ) );

        for ( DbLobReadTask lobTask : lobTasks )
            lobTask.start();
        for ( DbLobReadTask lobTask : lobTasks )
            lobTask.join();
        for ( DbLobReadTask lobTask : lobTasks ) {
            Assert.assertTrue( lobTask.isTaskSuccess(), lobTask.getErrorMsg() );
            int b = lobTask.getBegin();
            int length = lobTask.getLength();
            byte[] expect = Arrays.copyOfRange( expectBytes, b, b + length );
            byte[] actual = lobTask.getResult();
            RandomWriteLobUtil.assertByteArrayEqual( actual, expect );
        }
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
            db.close();
        }

    }

}
