using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.DataType
{

    /**
      * description: numberlong
      *              SEQUOIADBMAINSTREAM-2135
      * testcase:    13652 
      * author:      chensiqin
      * date:        2018/3/16
     */
    [TestClass]
    public class Numberlong13652
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl13652";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);

        }

        [TestMethod()] 
        public void TestNumberlong13652()
        {
            string expect1 = "";
            string expect2 = "";
            BsonDocument result = null;
            BsonDocument obj = new BsonDocument();

            obj.Add("a", 123).
                Add("b", int.MaxValue).
                Add("c", int.MinValue).
                Add("e", int.MaxValue + 1L).
                Add("f", int.MinValue - 1L).
                Add("g", long.MaxValue).
                Add("h", long.MinValue).
                Add("i", -9007199254740991).
                Add("j", 9007199254740991).
                Add("k", -(9007199254740992L - 1L)).
                Add("l", 9007199254740992L - 1L);
            cl.Insert(obj);
            DBCursor cur = cl.Query(null, new BsonDocument().Add("_id", new BsonDocument().Add("$include", 0)), null, null);
            try
            {
                result = cur.Next();
            }
            finally
            {
                cur.Close();
            }
            expect1 = "{ \"a\" : 123, \"b\" : 2147483647, \"c\" : -2147483648, \"e\" : 2147483648, \"f\" : -2147483649, \"g\" : 9223372036854775807, \"h\" : -9223372036854775808, \"i\" : -9007199254740991, \"j\" : 9007199254740991, \"k\" : -9007199254740991, \"l\" : 9007199254740991 }";
            expect2 = "{ \"a\" : 123, \"b\" : 2147483647, \"c\" : -2147483648, \"e\" : 2147483648, \"f\" : -2147483649, \"g\" : { \"$numberLong\": \"9223372036854775807\" }, \"h\" : { \"$numberLong\": \"-9223372036854775808\" }, \"i\" : -9007199254740991, \"j\" : 9007199254740991, \"k\" : -9007199254740991, \"l\" : 9007199254740991 }";

            BsonDefaults.JsCompatibility = true;
            Assert.AreEqual(expect2, result.ToString());

            BsonDefaults.JsCompatibility = false;
            Assert.AreEqual(expect1, result.ToString());

            BsonDefaults.JsCompatibility = true;
           Assert.AreEqual(expect2, result.ToString());
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
            catch (BaseException e)
            {
                Assert.Fail("Failed to clearup:", e.ErrorCode + e.Message);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
            }
        }

    }

}
