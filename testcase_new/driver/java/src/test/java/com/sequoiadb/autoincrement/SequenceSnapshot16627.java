package com.sequoiadb.autoincrement;

import java.util.ArrayList;
import java.util.List;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-16627
 * @describe: sequence快照/列表验证
 * @author wangkexin
 * @Date 2018.12.29
 * @version 1.00
 */
public class SequenceSnapshot16627 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "cl16627";
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
    }

    @Test
    public void test() {
        CreateAutoIncrementCL();
        TestGetSnapshot();
        TestGetList();
    }

    @AfterClass
    public void tearDown() {
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.close();
    }

    public void CreateAutoIncrementCL() {
        DBCollection cl = cs.createCollection( clName );
        List< BSONObject > optionsList = new ArrayList< BSONObject >();

        BSONObject options = new BasicBSONObject();
        options.put( "Field", "test16627_1" );
        options.put( "MinValue", 1 );
        options.put( "StartValue", 2 );
        optionsList.add( options );

        options = new BasicBSONObject();
        options.put( "Field", "test16627_2" );
        options.put( "MinValue", 2 );
        options.put( "StartValue", 5 );
        optionsList.add( options );

        options = new BasicBSONObject();
        options.put( "Field", "test16627_3" );
        options.put( "MinValue", 1 );
        options.put( "StartValue", 4 );
        optionsList.add( options );

        cl.createAutoIncrement( optionsList );
    }

    public void TestGetSnapshot() {
        List< BSONObject > autoIncrementInfos = Commlib.GetAutoIncrementList(
                sdb, SdbTestBase.csName + "." + clName, 3 );

        BSONObject obj = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject matcher = new BasicBSONObject();
        BSONObject orderBy = new BasicBSONObject();

        BasicBSONList sequenceNames = new BasicBSONList();
        for ( int i = 0; i < autoIncrementInfos.size(); i++ ) {
            BSONObject sequenceName = new BasicBSONObject();
            sequenceName.put( "Name", autoIncrementInfos.get( i )
                    .get( "SequenceName" ).toString() );
            sequenceNames.add( sequenceName );
        }
        matcher.put( "$or", sequenceNames );
        selector.put( "StartValue", 1 );
        orderBy.put( "StartValue", -1 );

        // getSnapshot(int snapType, BSONObject matcher, BSONObject selector,
        // BSONObject orderBy)
        DBCursor cur = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SEQUENCES, matcher,
                selector, orderBy );
        BasicBSONList expected = new BasicBSONList();
        BasicBSONList actual = new BasicBSONList();
        BSONObject expStartValue = new BasicBSONObject();
        expStartValue.put( "StartValue", 5 );
        expected.add( expStartValue );
        expStartValue = new BasicBSONObject();
        expStartValue.put( "StartValue", 4 );
        expected.add( expStartValue );
        expStartValue = new BasicBSONObject();
        expStartValue.put( "StartValue", 2 );
        expected.add( expStartValue );
        while ( cur.hasNext() ) {
            obj = cur.getNext();
            actual.add( obj );
        }
        cur.close();
        Assert.assertEquals( actual.toString(), expected.toString() );

        long skipRows = 1;
        long returnRows = 1;
        BSONObject hint = new BasicBSONObject();
        // getSnapshot(int snapType, BSONObject matcher, BSONObject selector,
        // BSONObject orderBy, BSONObject hint, long skipRows, long returnRows)
        cur = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SEQUENCES, matcher, selector,
                orderBy, hint, skipRows, returnRows );
        expected = new BasicBSONList();
        actual = new BasicBSONList();
        expStartValue = new BasicBSONObject();
        expStartValue.put( "StartValue", 4 );
        expected.add( expStartValue );
        while ( cur.hasNext() ) {
            obj = cur.getNext();
            actual.add( obj );
        }
        cur.close();
        Assert.assertEquals( actual.toString(), expected.toString() );

        // getSnapshot(int snapType, String matcher, String selector, String
        // orderBy)
        cur = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SEQUENCES,
                "{$or:" + sequenceNames.toString() + "}", "{StartValue:1}",
                "{StartValue:-1}" );
        expected = new BasicBSONList();
        actual = new BasicBSONList();
        expStartValue = new BasicBSONObject();
        expStartValue.put( "StartValue", 5 );
        expected.add( expStartValue );
        expStartValue = new BasicBSONObject();
        expStartValue.put( "StartValue", 4 );
        expected.add( expStartValue );
        expStartValue = new BasicBSONObject();
        expStartValue.put( "StartValue", 2 );
        expected.add( expStartValue );
        while ( cur.hasNext() ) {
            obj = cur.getNext();
            actual.add( obj );
        }
        cur.close();
        Assert.assertEquals( actual.toString(), expected.toString() );
    }

    public void TestGetList() {
        List< BSONObject > autoIncrementInfos = Commlib.GetAutoIncrementList(
                sdb, SdbTestBase.csName + "." + clName, 3 );

        BSONObject obj = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject query = new BasicBSONObject();
        BSONObject orderBy = new BasicBSONObject();

        BasicBSONList sequenceNames = new BasicBSONList();
        for ( int i = 0; i < autoIncrementInfos.size(); i++ ) {
            BSONObject sequenceName = new BasicBSONObject();
            sequenceName.put( "Name", autoIncrementInfos.get( i )
                    .get( "SequenceName" ).toString() );
            sequenceNames.add( sequenceName );
        }
        query.put( "$or", sequenceNames );
        selector.put( "Name", 1 );
        orderBy.put( "Name", -1 );

        DBCursor cur = sdb.getList( Sequoiadb.SDB_LIST_SEQUENCES, query,
                selector, orderBy );
        BasicBSONList expected = new BasicBSONList();
        BasicBSONList actual = new BasicBSONList();
        BSONObject expNameValue = new BasicBSONObject();
        for ( int i = autoIncrementInfos.size() - 1; i > -1; i-- ) {
            expNameValue = new BasicBSONObject();
            expNameValue.put( "Name", autoIncrementInfos.get( i )
                    .get( "SequenceName" ).toString() );
            expected.add( expNameValue );
        }
        while ( cur.hasNext() ) {
            obj = cur.getNext();
            actual.add( obj );
        }
        cur.close();
        Assert.assertEquals( actual.toString(), expected.toString() );
    }
}
