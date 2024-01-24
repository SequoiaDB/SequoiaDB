package com.sequoiadb.metadata;

import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.CommLib;
import java.util.ArrayList;

/**
 * @description seqDB-24902:cs.getDomainName获取当前集合所在domian
 * @author ZhangYanan
 * @createDate 2021.12.30
 * @updateUser ZhangYanan
 * @updateDate 2021.12.30
 * @updateRemark
 * @version v1.0
 */
public class TestDomain24902 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs = null;
    private String domainName = "cs_24902_Domain";
    private String csName = "cs_24902";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }
        ArrayList< String > groupInfo = CommLib.getDataGroupNames( sdb );
        BSONObject option = new BasicBSONObject();
        option.put( "Groups", groupInfo );
        sdb.createDomain( domainName, option );
        cs = sdb.createCollectionSpace( csName );
    }

    @Test
    public void test() {
        String csDomainName1 = cs.getDomainName();
        Assert.assertEquals( csDomainName1, "" );

        BSONObject option1 = new BasicBSONObject();
        option1.put( "Domain", domainName );
        cs.setDomain( option1 );
        String csDomainName2 = cs.getDomainName();
        Assert.assertEquals( csDomainName2, domainName );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            sdb.dropDomain( domainName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
