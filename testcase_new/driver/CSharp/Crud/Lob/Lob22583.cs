using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using SequoiaDB;
using SequoiaDB.Bson;
using CSharp.TestCommon;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace CSharp.Crud.Lob
{
    /**
    * description:   seqDB-22583:GetRunTimeDetail()接口验证 
    * author:        Zhao Xiaoni
    * date:          2020/8/8
    */

    [TestClass]
    public class Lob22583
    {
        private String clName = "cl_22583";
        private Sequoiadb sdb = null;
        private DBCollection cl = null;

        [TestInitialize]
        public void setup() 
        {
            Console.WriteLine( DateTime.Now.ToString( "yyyy-MM-dd hh:mm:ss:fff" ) + " begin: " + this.GetType().ToString() );
            sdb = new Sequoiadb( SdbTestBase.coordUrl );
            sdb.Connect();
            cl = sdb.GetCollecitonSpace(SdbTestBase.csName).CreateCollection( clName );
        }

        [TestMethod]
        public void lob22583()
        {
            int length = 1024;
            byte[] expect = LobUtils.GetRandomBytes( length );
            DBLob lob = cl.CreateLob();
            lob.Lock( 0, length );
            lob.Write( expect );

            BsonDocument actual_detail = lob.GetRunTimeDetail().GetElement( "AccessInfo" ).Value.AsBsonDocument;
            BsonDocument expect_detail = new BsonDocument();
            expect_detail.Add("RefCount", 1);
            expect_detail.Add( "ReadCount", 0 );
            expect_detail.Add("WriteCount", 0);
            expect_detail.Add("ShareReadCount", 0);
            BsonArray array = new BsonArray();
            expect_detail.Add( "LockSections", array );
            Console.WriteLine("actual_detail:"+actual_detail);
            Console.WriteLine("expect_detail:"+expect_detail);
            Assert.AreEqual( expect_detail, actual_detail );

            lob.Close();
        }

        [TestCleanup]
        public void tearDown() 
        {
           sdb.GetCollecitonSpace( SdbTestBase.csName ).DropCollection( clName ); 
           sdb.Disconnect();
           Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end: " + this.GetType().ToString());
        }
    }
}
