using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Lob
{
    /// <summary>
    /// TestLob20311 的摘要说明
    /// </summary>
    [TestClass]
    public class TestLob20311
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection mainCL = null;
        private DBCollection subCL = null;
        private const string mainClName = "maincl20311";
        private const string subClName = "subcl20311";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            
        }

        [TestMethod]
        public void Test20311()
        {
            try
            {
                BsonDocument options = new BsonDocument();
                options.Add("LobShardingKeyFormat", "YYYYMMDD");
                options.Add("ShardingKey", new BsonDocument("date", 1));
                options.Add("IsMainCL", true);
                options.Add("ShardingType", "range");
                mainCL = cs.CreateCollection(mainClName, options);
                subCL = cs.CreateCollection(subClName);

                options = new BsonDocument();
                options.Add("LowBound", new BsonDocument("date", "20190701"));
                options.Add("UpBound", new BsonDocument("date", "20190801"));
                mainCL.AttachCollection(SdbTestBase.csName + "." + subClName, options);

                byte[] testLobBuff = LobUtils.GetRandomBytes(100);
                ObjectId oid1 = subCL.CreateLobID();
                DBLob lob = subCL.CreateLob(oid1);
                lob.Close();

                DateTime dt = new DateTime();
                ObjectId oid2 = subCL.CreateLobID(dt);
                lob = subCL.CreateLob(oid2);
                lob.Close();

                BsonDocument selector = new BsonDocument();
                selector.Add("Oid", 1);
                DBCursor cursor = mainCL.ListLobs(null, selector, null, null, 0, -1);
                string expected1 = "{ \"Oid\" : { \"$oid\" : \""+oid1+"\" } }";
                string expected2 = "{ \"Oid\" : { \"$oid\" : \""+oid2+"\" } }";

                int count = 0;
                while (cursor.Next() != null)
                {
                    count++;
                    if (!expected1.Equals(cursor.Current().ToString()) && !expected2.Equals(cursor.Current().ToString()))
                    {
                        Assert.Fail(cursor.Current().ToString() + " equals " + expected1 + " or " + cursor.Current().ToString() + " equals " + expected2);
                    }
                }
                cursor.Close();
                Assert.AreEqual(2, count);
            }
            catch (BaseException e)
            {
                Assert.Fail("Test20311 fail" + e.Message);
            }
            
        }


        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(mainClName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }
    }
}
