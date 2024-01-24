package com.sequoiadb.bsontypes;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.Binary;
import org.bson.types.Symbol;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

import static org.testng.Assert.*;

/**
 * Created by laojingtang on 17-4-8. 覆盖的测试用例：11323 测试点：Symbol(String s)
 * getSymbol() equals(Object o) hashCode() toString()
 */
public class SymbolTest11323 extends SdbTestBase {
    private String clName = "cl_11323";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
    }

    @Test
    public void testBinary() {
        String str = "Hello World!";
        String str2 = "Hello World Again!";
        Symbol symbol = new Symbol( str );
        Symbol symbol1 = new Symbol( str2 );
        Symbol symbol2 = new Symbol( str );
        assertTrue( str.equals( symbol.getSymbol() ) );

        assertFalse( symbol.equals( symbol1 ) );
        assertTrue( symbol.equals( symbol2 ) );

        assertFalse( symbol.hashCode() == symbol1.hashCode() );
        assertTrue( symbol.hashCode() == symbol2.hashCode() );

        assertTrue( symbol.toString().equals( str ) );

    }

    @AfterClass
    public void tearDown() {
    }
}
