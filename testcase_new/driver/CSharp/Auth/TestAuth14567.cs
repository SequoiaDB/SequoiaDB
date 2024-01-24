using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Auth
{
    /**
     * description:  
     *                创建/删除鉴权用户 
     *                CreateUser (String username, String password)；RemoveUser (String username, String password)
     *                1.创建鉴权用户，查看操作结果 
     *                2、使用该鉴权用户连接数据库，执行插入、查询操作 
     *                3、删除鉴权用户，查看该用户是否存在 
     *                4、重复创建相同用户 
     * testcase:     seqDB-14567  seqDB-14568 
     * author:       chensiqin
     * date:         2019/03/26
    */

    [TestClass]
    public class TestAuth14567
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14567";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test14567()
        {
            if (Common.IsStandalone(sdb) == true ) 
            {
               Console.WriteLine("is standalone.");
               return ;
            }
            string username = "name14567";
            string passwd = "passwd14567";

            sdb.CreateUser(username, passwd);
            Sequoiadb localdb = new Sequoiadb(SdbTestBase.coordUrl);
            localdb.Connect(username, passwd);
            cs = localdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            BsonDocument record = new BsonDocument("a", 1);
            cl.Insert(record);
            DBCursor cur = cl.Query();
            int count = 0;
            while (cur.Next() != null)
            {
                Assert.AreEqual(record, cur.Current());
                count++;
            }
            cur.Close();
            Assert.AreEqual(1, count);
            //重复创建相同user
            try
            {
                localdb.CreateUser(username, passwd);
                Assert.Fail("excepet createUser failed. but ok");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-295, e.ErrorCode);
            }
            //删除鉴权用户
            localdb.RemoveUser(username, passwd);
            localdb.Connect();
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
