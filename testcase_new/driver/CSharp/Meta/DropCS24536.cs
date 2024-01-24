using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Meta
{
    /**
     * @Description seqDB-24536:dropCS指定options
     * @Author liuli
     * @Date 2021.10.29
     * @UpdatreAuthor liuli
     * @UpdateDate 2021.10.29
     * @version 1.00
    */
    [TestClass]
    public class DropCS24536
    {
        private Sequoiadb sdb = null;
        private string cs_Name = "cs_24536";
        private string cl_Name = "cl_24536";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test24536()
        {
            // cs下存在cl，指定option中EnsureEmpty为false
            CollectionSpace cs1 = sdb.CreateCollectionSpace(cs_Name);
            cs1.CreateCollection(cl_Name);
            sdb.DropCollectionSpace(cs_Name,new BsonDocument("EnsureEmpty",false));
            Assert.AreEqual(false,sdb.IsCollectionSpaceExist(cs_Name));

            // cs下不存在cl，指定option中EnsureEmpty为false
            sdb.CreateCollectionSpace(cs_Name);
            sdb.DropCollectionSpace(cs_Name, new BsonDocument("EnsureEmpty", false));
            Assert.AreEqual(false, sdb.IsCollectionSpaceExist(cs_Name));

            // cs下不存在cl，指定option中EnsureEmpty为true
            sdb.CreateCollectionSpace(cs_Name);
            sdb.DropCollectionSpace(cs_Name, new BsonDocument("EnsureEmpty", true));
            Assert.AreEqual(false, sdb.IsCollectionSpaceExist(cs_Name));

            // cs下存在cl，指定option中EnsureEmpty为true
            CollectionSpace cs2 = sdb.CreateCollectionSpace(cs_Name);
            cs2.CreateCollection(cl_Name);
            try 
            {
                sdb.DropCollectionSpace(cs_Name,new BsonDocument("EnsureEmpty", true));
                Assert.Fail("should error but success");
            }
            catch (BaseException e) 
            {
                Assert.AreEqual(-275, e.ErrorCode);
            }

            Assert.AreEqual(true, sdb.IsCollectionSpaceExist(cs_Name));
        }

        [TestCleanup()]
        public void TearDown()
        {
            try 
            { 
                sdb.DropCollectionSpace(cs_Name); 
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
