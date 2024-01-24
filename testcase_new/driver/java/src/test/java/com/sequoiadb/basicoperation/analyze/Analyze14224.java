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
public class Analyze14224 extends SdbTestBase {
    private SdbWarpper db;
    private SdbClWarpper dbcl;

    @BeforeClass
    public void setup() {
        db = new SdbWarpper( coordUrl );
        String pre = this.getClass().getSimpleName();
        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", 4096 );
        SdbCsProperties cs = new SdbCsProperties( pre + "cs", options );
        db.createCS( cs );
        SdbClProperties clPrope = SdbClProperties
                .newBuilder( cs, this.getClass().getSimpleName() ).build();
        dbcl = db.createCL( clPrope );
    }

    @AfterClass
    public void teardown() {
        db.dropCollectionSpace( dbcl.getCSName() );
        db.close();
    }

    /**
     * 1.创建1个cl，并创建多个索引，插入包含索引字段的数据，例如：插入数据包含a字段，值为0的2000条记录，任意数据2000条记录，其中a为索引字段
     * 2.执行统计，其中：参数指定cl及索引，在cl中使用索引字段执行查询，检查查询扫描方式，例如：使用a=0执行查询
     * 3.执行统计生成默认统计信息，其中：参数指定cl及索引，mode指定为3，在cl中使用索引字段执行查询，检查扫描方式，例如：使用a=0执行查询
     * <p>
     * 1.数据插入成功，索引创建成功 2.该索引使用表扫描，其他索引使用索引扫描 3.所有索引均使用索引扫描
     */
    @Test
    public void test() {
        // some records
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 25; i++ ) {
            records.add( new BasicBSONObject( "a", 0 ).append( "astr",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
            records.add( new BasicBSONObject( "b", 0 ).append( "bstr",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
            records.add( new BasicBSONObject( "c", 0 ).append( "cstr",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
            records.add( new BasicBSONObject( "d", 0 ).append( "dstr",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
            records.add( new BasicBSONObject( "e", 0 ).append( "estr",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
        }
        dbcl.insert( records );

        String indexkey[] = { "a", "b", "c", "d", "e" };
        String indexname[] = { "aIndex", "bIndex", "cIndex", "dIndex",
                "eIndex" };

        List< IndexProperties > indexPropertiesList = new ArrayList<>( 10 );
        for ( int i = 0; i < 5; i++ ) {
            IndexProperties index = IndexProperties
                    .newBuilder( indexname[ i ],
                            new BasicBSONObject( indexkey[ i ], 1 ) )
                    .enforced( false ).isUnique( false ).build();
            dbcl.createIndex( index );
            indexPropertiesList.add( index );
        }

        IndexProperties indexToAnalyze = indexPropertiesList.get( 0 );

        db.analyze( new BasicBSONObject( "Collection", dbcl.getFullName() )
                .append( "Index", indexToAnalyze.getIndexName() ) );

        for ( IndexProperties indexProperties : indexPropertiesList ) {
            String key = indexProperties.getIndexKey().keySet().iterator()
                    .next();
            Explain e = new Explain.Builder( dbcl )
                    .matcher( new BasicBSONObject( key, 0 ) )
                    .options( new BasicBSONObject( "Run", true ) ).build();
            if ( indexProperties == indexToAnalyze ) {
                assertTrue( e.isQueryUseTbscan(), e.getExplainResult() );
            } else {
                assertTrue( e.isQueryUseIxscan(), e.getExplainResult() );
            }
        }

        db.analyze( new BasicBSONObject( "Collection", dbcl.getFullName() )
                .append( "Mode", 3 )
                .append( "Index", indexToAnalyze.getIndexName() ) );

        // query should use idxscan
        for ( IndexProperties indexProperties : indexPropertiesList ) {
            String key = indexProperties.getIndexKey().keySet().iterator()
                    .next();
            Explain e = new Explain.Builder( dbcl )
                    .matcher( new BasicBSONObject( key, 0 ) )
                    .options( new BasicBSONObject( "Run", true ) ).build();
            e.execute();
            assertTrue( e.isQueryUseIxscan(), e.getExplainResult() );
        }
    }
}
