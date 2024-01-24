package com.sequoiadb.basicoperation.analyze;

import com.sequoiadb.testcommon.SdbTestBase;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

import static org.testng.Assert.assertTrue;

/**
 * Created by laojingtang on 18-1-31.
 */
public class Analyze14223 extends SdbTestBase {
    private SdbWarpper db;
    private List< SdbClWarpper > dbcls = new ArrayList<>( 10 );

    @BeforeClass
    public void setup() {
        db = new SdbWarpper( coordUrl );
        String pre = this.getClass().getSimpleName();
        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", 4096 );
        for ( int i = 0; i < 5; i++ ) {
            SdbCsProperties cs = new SdbCsProperties( pre + "cs" + i, options );
            db.createCS( cs );
            SdbClProperties cl = SdbClProperties.newBuilder( cs, "cl" ).build();
            dbcls.add( db.createCL( cl ) );
        }
    }

    @AfterClass
    public void teardown() {
        for ( SdbClWarpper dbcl : dbcls ) {
            db.dropCollectionSpace( dbcl.getCSName() );
        }
        db.close();
    }

    /**
     * 1.创建1个cs，并创建多个cl，在各cl上创建索引，插入包含索引字段的数据，例如：插入数据包含a字段，值为0的2000条记录，任意数据2000条记录，其中a为索引字段
     * 2.在各cl中使用索引字段执行查询，检查扫描方式，例如：使用a=0执行查询
     * 3.执行统计，其中：参数指定cl，执行步骤2中的查询，检查扫描方式，例如：使用a=0执行查询
     * <p>
     * 1.数据插入成功，索引创建成功 2.所有cl均使用索引执行查询 3.其他cl使用索引查询，统计后的cl使用表扫描执行查询
     */
    @Test
    public void test() {
        // some records
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 25; i++ ) {
            records.add( new BasicBSONObject( "a", 0 ).append( "b",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
        }
        // create cs cl
        for ( SdbClWarpper cl : dbcls ) {
            cl.insert( records );
            cl.createIndex( "aIndex", "{a:1}", false, false );
        }

        // query should use idxscan
        for ( SdbClWarpper dbcl : dbcls ) {
            Explain e = new Explain.Builder( dbcl )
                    .matcher( new BasicBSONObject( "a", 0 ) )
                    .options( new BasicBSONObject( "Run", true ) ).build();
            assertTrue( e.isQueryUseIxscan(), e.getExplainResult() );
        }

        SdbClWarpper analyzeCl = dbcls.get( 0 );
        db.analyze(
                new BasicBSONObject( "Collection", analyzeCl.getFullName() ) );

        // query should use tbscan
        for ( SdbClWarpper dbcl : dbcls ) {
            Explain e = new Explain.Builder( dbcl )
                    .matcher( new BasicBSONObject( "a", 0 ) )
                    .options( new BasicBSONObject( "Run", true ) ).build();
            if ( dbcl == analyzeCl ) {
                assertTrue( e.isQueryUseTbscan(), e.getExplainResult() );
            } else {
                assertTrue( e.isQueryUseIxscan(), e.getExplainResult() );
            }
        }
    }
}
