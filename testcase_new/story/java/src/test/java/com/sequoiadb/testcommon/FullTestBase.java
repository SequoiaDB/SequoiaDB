/**
 * Copyright (c) 2019, SequoiaDB Ltd.
 * File Name:FullTestBase.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2019-7-26上午9:56:05
 *  @version 1.00
 */
package com.sequoiadb.testcommon;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.ITestResult;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;

import java.util.Properties;
import java.util.concurrent.atomic.AtomicBoolean;

public class FullTestBase extends SdbTestBase {
    protected Sequoiadb sdb = null;
    protected DBCollection cl = null;
    private AtomicBoolean caseFailed = new AtomicBoolean( false );
    protected Properties caseProp = new Properties();

    protected static final String IGNORESTANDALONE = "ignorestandalone";
    protected static final String IGNOREONEGROUP = "ignoreonegroup";
    protected static final String DOMAIN = "domain";
    protected static final String DOMAINOPT = "domainOpt";
    protected static final String CSNAME = "csName";
    protected static final String CLNAME = "clName";
    protected static final String CSOPT = "csOpt";
    protected static final String CLOPT = "clOpt";

    protected void initTestProp() {
    }

    protected void caseInit() throws Exception {
    }

    /**
     * 配置 caseProp 中的 IGNORESTANDALONE|IGNOREONEGROUP 参数，是否跳过独立模式 (default:true)
     * 和单组模式 (default:false)
     */
    private void skipTest() {
        boolean isIgnoreStandAlone = Boolean.parseBoolean(
                caseProp.getProperty( IGNORESTANDALONE, "true" ) );
        if ( isIgnoreStandAlone && CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Standalone Mode Skip Testcase" );
        }

        boolean isIngoreOneGroup = Boolean.parseBoolean(
                caseProp.getProperty( IGNOREONEGROUP, "false" ) );
        if ( isIngoreOneGroup && CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "One Group Mode Skip Testcase" );
        }
    }

    /**
     * 配置 caseProp 中的 DOMAIN|DOMAINOPT 参数，在 caseInit 之前创建 doMain，存在 doMain
     * 则先删除后在创建
     */
    private void createDoMain() {
        String doMainName = caseProp.getProperty( DOMAIN );
        if ( doMainName != null ) {
            if ( sdb.isDomainExist( doMainName ) ) {
                sdb.dropDomain( doMainName );
            }
            String domainOpt = caseProp.getProperty( DOMAINOPT );
            sdb.createDomain( doMainName,
                    ( BSONObject ) JSON.parse( domainOpt ) );
        }
    }

    /**
     * 配置 caseProp 中的 CSNAME|CSOPT 参数，在 caseInit 之前创建 CS，存在 CS 直接返回，不存在则创建
     * 
     * @return
     */
    private CollectionSpace createCS() {
        CollectionSpace cs = null;
        String csName = caseProp.getProperty( CSNAME, SdbTestBase.csName );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            cs = sdb.getCollectionSpace( csName );
        } else {
            String csOpt = caseProp.getProperty( CSOPT );
            if ( csOpt == null ) {
                cs = sdb.createCollectionSpace( csName );
            } else {
                cs = sdb.createCollectionSpace( csName,
                        ( BSONObject ) JSON.parse( csOpt ) );
            }
        }
        return cs;
    }

    private DBCollection createCL( CollectionSpace cs ) {
        String clName = caseProp.getProperty( CLNAME );
        if ( clName != null ) {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            String clOpt = caseProp.getProperty( CLOPT );
            cl = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( clOpt ) );
            return cl;
        } else {
            return null;
        }
    }

    @BeforeClass
    public void setUp() throws Exception {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        initTestProp();
        skipTest();
        createDoMain();
        CollectionSpace cs = createCS();
        createCL( cs );
        caseInit();
    }

    @AfterMethod
    public void clearAfterMethod( ITestResult result ) {
        if ( !result.isSuccess() ) {
            caseFailed.set( true );
        }
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            if ( !caseFailed.get() ) {
                String csName = caseProp.getProperty( CSNAME,
                        SdbTestBase.csName );
                if ( SdbTestBase.csName.equals( csName ) ) {
                    CollectionSpace cs = sdb.getCollectionSpace( csName );
                    String clName = caseProp.getProperty( CLNAME );
                    if ( clName != null ) {
                        if ( cs.isCollectionExist( clName ) ) {
                            FullTextDBUtils.dropCollection( cs, clName );
                        }
                    }
                } else {
                    if ( sdb.isCollectionSpaceExist( csName ) ) {
                        FullTextDBUtils.dropCollectionSpace( sdb, csName );
                    }
                }
                String doMainName = caseProp.getProperty( DOMAIN );
                if ( doMainName != null ) {
                    if ( sdb.isDomainExist( doMainName ) ) {
                        sdb.dropDomain( doMainName );
                    }
                }
                caseFini();
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    protected void caseFini() throws Exception {

    }

}
