package com.sequoiadb.basicoperation;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.options.InsertOption;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-30389:insert指定flag为FLAG_INSERT_CONTONDUP_ID/FLAG_INSERT_REPLACEONDUP_ID
 * @Author Cheng Jingjing
 * @CreateDate 2023/3/8
 * @UpdateUser Cheng Jingjing
 * @UpdateDate 2023/3/8
 * @UpdateRemark
 * @Version 1.0
 */
public class testInsertFlags30389 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_30389";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
    }

    @Test
    private void testInsert() {
        BSONObject existData = new BasicBSONObject();
        existData.put( "_id", 1 );
        existData.put( "a", 1 );
        cl.insertRecord( existData );

        // case1: flag取值为SDB_INSERT_CONTONDUP_ID
        BSONObject ignoreData = new BasicBSONObject();
        ignoreData.put( "_id", 1 );
        ignoreData.put( "a", 222 );
        InsertOption option = new InsertOption();
        option.appendFlag( InsertOption.FLG_INSERT_CONTONDUP_ID );
        cl.insertRecord( ignoreData, option );
        DBCursor cursor = cl.query();
        while ( cursor.hasNext() ) {
            Assert.assertEquals( cursor.getNext(), existData );
        }
        cursor.close();

        // case2: flag取值为
        BSONObject replaceData = new BasicBSONObject();
        replaceData.put( "_id", 1 );
        replaceData.put( "a", 333 );
        InsertOption option2 = new InsertOption();
        option2.appendFlag( InsertOption.FLG_INSERT_REPLACEONDUP_ID );
        cl.insertRecord( replaceData, option2 );
        cursor = cl.query();
        while ( cursor.hasNext() ) {
            Assert.assertEquals( cursor.getNext(), replaceData );
        }
        cursor.close();
    }

    @AfterClass
    private void tearDown() {
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
