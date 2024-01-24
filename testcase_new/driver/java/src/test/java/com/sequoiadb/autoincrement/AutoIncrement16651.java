package com.sequoiadb.autoincrement;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-16651
 * @describe: 创建集合时指定自增字段，并修改自增字段属性
 * @author wangkexin
 * @Date 2018.12.29
 * @version 1.00
 */
public class AutoIncrement16651 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "cl16651";
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
        AlterAutoIncrementCL();
        SetAttributesAutoIncrementCL();
    }

    @AfterClass
    public void tearDown() {
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.close();
    }

    public void CreateAutoIncrementCL() {
        BSONObject options = new BasicBSONObject();
        BSONObject subOptions = new BasicBSONObject();
        subOptions.put( "Field", "number16651" );
        subOptions.put( "StartValue", 3 );
        options.put( "AutoIncrement", subOptions );

        cs.createCollection( clName, options );
        BSONObject autoIncrementInfo = Commlib.GetAutoIncrement( sdb,
                SdbTestBase.csName + "." + clName );
        Assert.assertEquals( autoIncrementInfo.get( "Field" ).toString(),
                "number16651" );
        String sequenceName = autoIncrementInfo.get( "SequenceName" )
                .toString();
        BSONObject actOptions = new BasicBSONObject();
        actOptions = Commlib.GetSequenceSnapshot( sdb, sequenceName );
        Assert.assertEquals( actOptions.get( "StartValue" ).toString(), "3" );
    }

    public void AlterAutoIncrementCL() {
        DBCollection cl = cs.getCollection( clName );
        BSONObject options = new BasicBSONObject();
        BSONObject subOptions = new BasicBSONObject();
        subOptions.put( "Field", "number16651" );
        subOptions.put( "StartValue", 2 );
        subOptions.put( "Increment", 3 );
        options.put( "AutoIncrement", subOptions );

        cl.alterCollection( options );
        BSONObject autoIncrementInfo = Commlib.GetAutoIncrement( sdb,
                SdbTestBase.csName + "." + clName );
        Assert.assertEquals( autoIncrementInfo.get( "Field" ).toString(),
                "number16651" );
        String sequenceName = autoIncrementInfo.get( "SequenceName" )
                .toString();
        BSONObject actOptions = new BasicBSONObject();
        actOptions = Commlib.GetSequenceSnapshot( sdb, sequenceName );
        Assert.assertEquals( actOptions.get( "StartValue" ).toString(), "2" );
        Assert.assertEquals( actOptions.get( "Increment" ).toString(), "3" );
    }

    public void SetAttributesAutoIncrementCL() {
        DBCollection cl = cs.getCollection( clName );
        BSONObject options = new BasicBSONObject();
        BSONObject subOptions = new BasicBSONObject();
        subOptions.put( "Field", "number16651" );
        subOptions.put( "StartValue", 2 );
        subOptions.put( "Increment", 3 );
        subOptions.put( "MinValue", 2 );
        options.put( "AutoIncrement", subOptions );

        cl.setAttributes( options );
        BSONObject autoIncrementInfo = Commlib.GetAutoIncrement( sdb,
                SdbTestBase.csName + "." + clName );
        Assert.assertEquals( autoIncrementInfo.get( "Field" ).toString(),
                "number16651" );
        String sequenceName = autoIncrementInfo.get( "SequenceName" )
                .toString();
        BSONObject actOptions = new BasicBSONObject();
        actOptions = Commlib.GetSequenceSnapshot( sdb, sequenceName );
        Assert.assertEquals( actOptions.get( "StartValue" ).toString(), "2" );
        Assert.assertEquals( actOptions.get( "Increment" ).toString(), "3" );
        Assert.assertEquals( actOptions.get( "MinValue" ).toString(), "2" );
    }
}
