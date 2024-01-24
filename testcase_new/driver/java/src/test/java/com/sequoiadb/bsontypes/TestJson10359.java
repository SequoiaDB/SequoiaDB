package com.sequoiadb.bsontypes;

import org.bson.BasicBSONCallback;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;

public class TestJson10359 {

    @Test
    public void testJson() {
        try {
            Object o = "{Name: foo.bar}";
            StringBuilder sb = new StringBuilder();
            JSON.serialize( o, sb );
            Assert.assertEquals( sb.toString(), "\"{Name: foo.bar}\"" );
            Assert.assertEquals( JSON.serialize( o ), "\"{Name: foo.bar}\"" );
            o = "{ \"Name\" : \"foo.bar\" }";
            Assert.assertEquals(
                    ( JSON.parse( "{\"Name\": \"foo.bar\"}" ) ).toString(),
                    o.toString() );
            Assert.assertEquals(
                    ( JSON.parse( "{\"Name\": \"foo.bar\"}",
                            new BasicBSONCallback() ) ).toString(),
                    o.toString() );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestJson10359 test error, error description:"
                            + e.getMessage() );
        }
    }
}
