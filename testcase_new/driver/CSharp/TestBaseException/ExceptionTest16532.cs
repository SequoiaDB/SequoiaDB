using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.TestBaseException
{
    /**
     * description:  使用无效更新符更新记录，查看getLastErrObj返回的错误对象
     * testcase:     seqDB-16532
     * author:       chensiqin
     * date:         2018/11/13
    */
    [TestClass]
    public class ExceptionTest16532
    {

        private Sequoiadb sdb = null;
        private CollectionSpace cs;
        private DBCollection cl;
        private string clName = "cl16532";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test16532()
        {
            if (isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = getDataGroupNames(sdb);
            createCL();
            DBQuery query = new DBQuery();
            BsonDocument modifier = new BsonDocument();
            BsonDocument obj = new BsonDocument();
            obj.Add("a", 2);
            modifier.Add("$seta", obj);
            query.Modifier = modifier;
            try
            {
                cl.Update(query);
            }
            catch (BaseException e)
            {
                BsonDocument errorObject = e.ErrorObject;
                Assert.AreEqual(-6, errorObject.GetElement("errno").Value);
                Assert.AreEqual("Invalid Argument", errorObject.GetElement("description").Value);
                ReplicaGroup rg = sdb.GetReplicaGroup(dataGroupNames[0]);
                string expected = "[{ \"NodeName\" : \"" + rg.GetMaster().NodeName + "\", \"GroupName\" : \"" + dataGroupNames[0] + "\", \"Flag\" : -6, \"ErrInfo\" : { \"UpdatedNum\" : 0, \"ModifiedNum\" : 0, \"InsertedNum\" : 0, \"errno\" : -6, \"description\" : \"Invalid Argument\", \"detail\" : \"Updator operator[$seta] error\" } }]";
 
                Assert.AreEqual(expected, errorObject.GetElement("ErrNodes").Value.ToString());
            }
            cs.DropCollection(clName);
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }

        public void createCL()
        {
            try
            {
                if (!sdb.IsCollectionSpaceExist(SdbTestBase.csName))
                {
                    sdb.CreateCollectionSpace(SdbTestBase.csName);
                }
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-33, e.ErrorCode);
            }
            try
            {

                cs = sdb.GetCollectionSpace(SdbTestBase.csName);
                BsonDocument option = new BsonDocument();
                option.Add("Group", dataGroupNames[0]);
                cl = cs.CreateCollection(clName, option);
            }
            catch (BaseException e)
            {
                Assert.IsTrue(false, "create cl fail " + e.Message);
            }
        }

        public static List<string> getDataGroupNames(Sequoiadb sdb)
        {
            List<string> list = new List<string>();
            BsonDocument matcher = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            BsonDocument orderBy = new BsonDocument();
            matcher.Add("Role", 0);
            selector.Add("GroupName", "");
            DBCursor cursor = sdb.GetList(7, matcher, selector, null);
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                BsonElement element = doc.GetElement("GroupName");
                list.Add(element.Value.ToString());
            }
            cursor.Close();
            return list;
        }

        public static bool isStandalone(Sequoiadb sdb)
        {
            bool flag = false;
            try
            {
                sdb.ListReplicaGroups();
            }
            catch (BaseException e)
            {
                if (e.ErrorCode == -159)
                {
                    flag = true;
                }
            }
            return flag;
        }
    }
}

