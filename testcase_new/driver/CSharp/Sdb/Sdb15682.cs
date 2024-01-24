using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Sdb
{
    /**
    * description: closeAllCursors  IsValid                 
    * testcase:    15682
    * author:      csq
    * date:        2018/08/29
    */

    [TestClass]
    public class Sdb15682
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl15682";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
        }

        [TestMethod]
        public void Test15682()
        {
             BsonDocument obj;
            for (int i = 0; i < 5; i++)
            {
                obj = new BsonDocument();
                obj.Add("age", i);
                cl.Insert(obj);
            }
            DBCursor cur1 = cl.Query();
            DBCursor cur2 = cl.Query();

            sdb.CloseAllCursors();
            try
            {
                cur1.Next();
                Assert.Fail("expected thorw -36 but success!");
            }
            catch(BaseException e)
            {
                Assert.AreEqual(-36, e.ErrorCode);
            }

            try
            {
                cur2.Next();
                Assert.Fail("expected thorw -36 but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-36, e.ErrorCode);
            }
            DBCursor cur3 = cl.Query();
            DBCursor cur4 = cl.Query();
            DBCursor cur5 = cl.Query();
            sdb.Interrupt();
            try
            {
                cur3.Next();
                Assert.Fail("expected thorw -36 but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-36, e.ErrorCode);
            }
            try
            {
                cur4.Next();
                Assert.Fail("expected thorw -36 but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-36, e.ErrorCode);
            }
            try
            {
                cur5.Next();
                Assert.Fail("expected thorw -36 but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-36, e.ErrorCode);
            }
            Assert.AreEqual(true, sdb.IsValid());
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                    Assert.AreEqual(false, sdb.IsValid());
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }
    }
}

