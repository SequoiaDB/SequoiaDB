package com.sequoiadb.basicoperation;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Descreption seqDB-23299:向中文集合空间，强转方式插入记录报溢出异常
 * @Author Yipan
 * @Date 2021.1.22
 */
public class TestInsertBson23299 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csname = "测试cs23299";
    private String clname = "测试cl23299";

    @BeforeClass
    public void setSdb() {
        // 建立 SequoiaDB 数据库连接
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csname ) ) {
            sdb.dropCollectionSpace( csname );
        }
    }

    @Test
    public void test() {
        // 创建中文名cs 创建Cl
        CollectionSpace cs = sdb.createCollectionSpace( csname );
        DBCollection cl = cs.createCollection( clname );
        // 预期结果
        ArrayList< BSONObject > expectedResults = new ArrayList< BSONObject >();
        // 实际结果
        ArrayList< BSONObject > actualResults = new ArrayList< BSONObject >();
        // 创建帮助对象
        DBCursor act;

        // insert
        String insert = "{ 'key':'value值' }";
        BSONObject obj = ( BSONObject ) JSON.parse( insert );
        cl.insert( obj );
        expectedResults.add( obj );
        act = cl.query();
        while ( act.hasNext() ) {
            actualResults.add( act.getNext() );
        }
        Assert.assertEquals( expectedResults, actualResults );
        expectedResults.clear();
        actualResults.clear();

        // update
        cl.insert( "{'a':1,'数量':3}" );
        cl.insert( "{'数量':2}" );
        BSONObject matcher = ( BSONObject ) JSON.parse( "{ 'a':1 }" );
        BSONObject modifier = new BasicBSONObject();
        BSONObject object = ( BSONObject ) JSON.parse( "{ '数量':4 }" );
        modifier.put( "$inc", object );
        cl.upsert( matcher, modifier, null );
        expectedResults.add( ( BSONObject ) JSON.parse( "{'key':'value值'}" ) );
        expectedResults.add( ( BSONObject ) JSON.parse( "{a:1,'数量':7}" ) );
        expectedResults.add( ( BSONObject ) JSON.parse( "{'数量':2}" ) );
        BSONObject selector = new BasicBSONObject( "_id",
                new BasicBSONObject( "$include", 0 ) );
        act = cl.query( null, selector, null, null );
        while ( act.hasNext() ) {
            actualResults.add( act.getNext() );
        }
        Assert.assertEquals( actualResults, expectedResults );
        expectedResults.clear();
        actualResults.clear();

        // query
        BSONObject query = ( BSONObject ) JSON.parse( "{'key':'value值'}" );
        act = cl.query( query, null, null, null );
        expectedResults.add( obj );
        while ( act.hasNext() ) {
            actualResults.add( act.getNext() );
        }
        Assert.assertEquals( actualResults, expectedResults );
        expectedResults.clear();
        actualResults.clear();

        // aggregate
        List< BSONObject > aggregateObj = new ArrayList< BSONObject >();
        BSONObject limitObj = ( BSONObject ) JSON.parse( "{$limit:5}" );
        BSONObject projectObj = ( BSONObject ) JSON
                .parse( "{$project:{a: 0,数量: 1}}" );
        aggregateObj.add( limitObj );
        aggregateObj.add( projectObj );
        act = cl.aggregate( aggregateObj );
        while ( act.hasNext() ) {
            actualResults.add( act.getNext() );
        }
        expectedResults.add( ( BSONObject ) JSON.parse( "{'数量':null}" ) );
        expectedResults.add( ( BSONObject ) JSON.parse( "{'数量':7}" ) );
        expectedResults.add( ( BSONObject ) JSON.parse( "{'数量':2}" ) );
        Assert.assertEquals( actualResults, expectedResults );
    }

    @AfterClass
    public void closeSdb() {
        sdb.dropCollectionSpace( csname );
        sdb.close();
    }
}