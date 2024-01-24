package com.sequoiadb.fulltext.largedata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * @Description seqDB-14372:无存量数据，插入记录
 * @author yinzhen
 * @date 2018/11/19
 */
public class Fulltext14372 extends FullTestBase {
    private String clName = "insertRecords14372";
    private String fullIndexName = "fullIndex14372";
    private String cappedName = null;
    private String esIndexName = null;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );

        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( fullIndexName,
                "{\"a\":\"text\",\"b\":\"text\",\"c\":\"text\",\"d\":\"text\",\"e\":\"text\",\"g\":\"text\"}",
                false, false );
        cappedName = FullTextDBUtils.getCappedName( cl, fullIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIndexName );
    }

    @Test
    public void test() throws Exception {
        insertData( FullTextUtils.INSERT_NUMS );// insert >128M
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIndexName,
                FullTextUtils.INSERT_NUMS ) );
        insertData( FullTextUtils.INSERT_NUMS ); // insert again
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIndexName,
                FullTextUtils.INSERT_NUMS * 2 ) );
    }

    @Override
    protected void caseFini() throws Exception {
        // check fulltext deleted
        if ( esIndexName != null ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
        }
    }

    private void insertData( int insertNums ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                BSONObject record = ( BSONObject ) JSON
                        .parse( "{a: 'test_14372_" + i * j + "', b: '"
                                + StringUtils.getRandomString( 64 ) + "', c: '"
                                + StringUtils.getRandomString( 64 ) + "', d: '"
                                + StringUtils.getRandomString( 64 ) + "', e: '"
                                + StringUtils.getRandomString( 128 ) + "', g: '"
                                + StringUtils.getRandomString( 256 ) + "'}" );
                records.add( record );
            }
            cl.insert( records );
            records.clear();
        }
    }

}
