package com.sequoiadb.split.serial;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.threadexecutor.annotation.ExpectBlock;

/**
 * jira-4318
 * 
 * @description seqDB-11558:数组进行切分
 *              插入分区键字段为数组且包含多个元素的记录，然后百分比切分，卡住后删除包含多个元素的数组记录，检查结果
 *              再次插入包含多个元素的数组记录,检查结果 更新分区键为多个元素的数组记录，检查结果
 * @author huangxiaoni
 * @date 2019.3.21
 * @review
 */

public class SplitHash11558B extends SdbTestBase {
    private Sequoiadb sdb;
    private String srcRg;
    private String dstRg;
    private CollectionSpace cs;
    private final String CL_NAME_BASE = "cl_hash_11558_B";
    private ArrayList< DBCollection > cls = new ArrayList<>();
    private ArrayList< String > clNames = new ArrayList<>();
    private ArrayList< Object > validDataArr = new ArrayList<>();
    private ArrayList< Object > invalidDataArr = new ArrayList<>();
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone, or only one group, skip the testCase." );
        }

        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcRg = groupNames.get( 0 );
        dstRg = groupNames.get( 1 );

        cs = sdb.getCollectionSpace( SdbTestBase.csName );

        this.readySampleData();

        BSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "hash" );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "Group", srcRg );
        for ( int i = 0; i < invalidDataArr.size(); i++ ) {
            String clName = CL_NAME_BASE + "_" + i;
            clNames.add( clName );

            DBCollection cl = cs.createCollection( clName, options );
            cls.add( cl );
        }
    }

    @Test
    private void test() throws Exception {
        for ( int i = 0; i < invalidDataArr.size(); i++ ) {
            String clName = clNames.get( i );
            DBCollection cl = cls.get( i );
            System.out.println( "clName: " + clName );

            // insert multiple valid sharding key
            for ( int j = 0; j < validDataArr.size(); j++ ) {
                BSONObject vDoc = new BasicBSONObject();
                vDoc.put( "a", validDataArr.get( j ) );
                vDoc.put( "b", "valid" );
                vDoc.put( "c", j );
                cl.insert( vDoc );
            }

            // insert one invalid sharding key
            BSONObject invDoc = new BasicBSONObject();
            invDoc.put( "a", invalidDataArr.get( i ) );
            invDoc.put( "b", "invalid" );
            cl.insert( invDoc );

            // percent split
            ThreadExecutor es = new ThreadExecutor();
            es.addWorker( new percentSplit( clName, 50 ) );
            es.addWorker( new deleteInvalidRecs( clName, invDoc ) );
            es.run();

            DBCursor cursor = cl.query( invDoc, null, null, null );
            Assert.assertFalse( cursor.hasNext() );

            // insert again
            try {
                cl.insert( invDoc );
            } catch ( BaseException e ) {
                if ( -174 != e.getErrorCode() ) {
                    throw e;
                }
            }

            // update
            BSONObject modifier = new BasicBSONObject();
            modifier.put( "$set",
                    new BasicBSONObject( "a", invDoc.get( "a" ) ) );
            cl.update( null, modifier, null );

            // upsert
            try {
                modifier = new BasicBSONObject();
                modifier.put( "$set",
                        new BasicBSONObject( "a", invDoc.get( "a" ) ) );
                cl.upsert( new BasicBSONObject( "b", "test" ), modifier, null );
            } catch ( BaseException e ) {
                if ( -174 != e.getErrorCode() ) {
                    throw e;
                }
            }

            // check by $all
            this.checkValidSrdRecs( cl );

            long totalCnt = cl.getCount();
            Assert.assertEquals( totalCnt, validDataArr.size() );

            this.checkGroups( cl );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( int i = 0; i < clNames.size(); i++ ) {
                    cs.dropCollection( clNames.get( i ) );
                }
            } else {
                for ( int i = 0; i < clNames.size(); i++ ) {
                    DBCollection cl = cs.getCollection( clNames.get( i ) );
                    DBCursor cursor = cl.query();
                    ArrayList< BSONObject > recsArr = new ArrayList<>();
                    while ( cursor.hasNext() ) {
                        BSONObject recs = cursor.getNext();
                        recsArr.add( recs );
                    }
                    System.out.println( "teardown query " + clNames.get( i )
                            + ": " + recsArr );
                }
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class percentSplit {
        private String clName;
        private int percent;

        private percentSplit( String clName, int percent ) {
            this.clName = clName;
            this.percent = percent;
        }

        @ExecuteOrder(step = 1)
        @ExpectBlock(confirmTime = 5, contOnStep = 2)
        private void splitOper() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                System.out.println( new Date() + " begin to split[" + clName
                        + "], srcRg[" + srcRg + "], dstRg[" + dstRg + "]" );
                db.msg( "begin to split[" + clName + "], srcRg[" + srcRg
                        + "], dstRg[" + dstRg + "]" );

                cl.split( srcRg, dstRg, percent );

                db.msg( "split end, " + clName );
                System.out.println( new Date() + " split end, " + clName );
            } finally {
                if ( db != null )
                    db.close();
            }
        }
    }

    private class deleteInvalidRecs {
        private String clName;
        private BSONObject matcher;

        private deleteInvalidRecs( String clName, BSONObject matcher ) {
            this.clName = clName;
            this.matcher = matcher;
        }

        @ExecuteOrder(step = 2)
        private void deleteOper() throws InterruptedException {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                System.out.println( new Date() + " begin to delete " + clName );
                db.msg( "begin to delete " + clName );

                cl.delete( matcher );

                db.msg( "delete end, " + clName );
                System.out.println( new Date() + " delete end, " + clName );

                DBCursor cr = cl.query( matcher, null, null, null );
                Assert.assertFalse( cr.hasNext() );
            } finally {
                if ( db != null )
                    db.close();
            }
        }
    }

    private void readySampleData() {
        // valid shardingKey
        int a = 0;
        validDataArr.add( a );

        ArrayList< Integer > arr = new ArrayList<>();
        validDataArr.add( arr );

        arr = new ArrayList<>();
        arr.add( 1 );
        validDataArr.add( arr );

        arr = new ArrayList<>();
        ArrayList< Object > objArr = new ArrayList<>();
        ArrayList< Integer > embArr = new ArrayList<>();
        embArr.add( 1 );
        embArr.add( 2 );
        embArr.add( 9 );
        objArr.add( embArr );
        validDataArr.add( objArr );

        // invalid shardingKey
        arr = new ArrayList<>();
        arr.add( 1 );
        arr.add( 9 );
        invalidDataArr.add( arr );

        arr = new ArrayList<>();
        arr.add( 9 );
        arr.add( 1 );
        invalidDataArr.add( arr );

        arr = new ArrayList<>();
        arr.add( 1 );
        arr.add( 2 );
        arr.add( 9 );
        invalidDataArr.add( arr );
    }

    private void checkValidSrdRecs( DBCollection cl ) {
        for ( int i = 0; i < validDataArr.size(); i++ ) {
            BSONObject doc = new BasicBSONObject();
            if ( i == 0 ) {
                doc.put( "a", validDataArr.get( i ) );
            } else {
                doc.put( "a",
                        new BasicBSONObject( "$all", validDataArr.get( i ) ) );
            }
            doc.put( "b", "valid" );
            doc.put( "c", i );
            DBCursor rc = cl.query( doc, null, null, null );
            int num = 0;
            while ( rc.hasNext() ) {
                BSONObject rcDoc = rc.getNext();
                Assert.assertEquals( rcDoc.get( "a" ), validDataArr.get( i ) );
                num++;
            }
            Assert.assertEquals( num, 1 );
        }
    }

    private void checkGroups( DBCollection cl ) {
        DBCursor cursor = sdb.getSnapshot( 8,
                new BasicBSONObject( "Name", cl.getFullName() ), null, null );
        BasicBSONObject info = ( BasicBSONObject ) cursor.getNext();
        BasicBSONList cataInfo = ( BasicBSONList ) info.get( "CataInfo" );
        Assert.assertEquals( cataInfo.size(), 2 );
    }
}
