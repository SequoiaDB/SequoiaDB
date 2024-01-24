using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Cluster
{
    /**
     * description:   connect () connect (string username, string password) 
     *                connect (string username, string password，ConfigOptions options)
     *                disconnect () IsValid（）
     *  	          1、连接远程sequoiadb，不指定用户名和密码，并创建集合1 
     *  	          2、连接远程sequoiadb，指定用户名和密码，并创建集合2 
     *  	          3、连接远程sequoiadb，指定用户名、密码、options参数等，并创建集合3 
     *  	          4、disconnect节点，检查是否断连（IsValid判断） 
     * testcase:    14576
     * author:      chensiqin
     * date:        2019/04/15
     */

    [TestClass]
    public class TestConnect14576
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName1 = "cl14576_1";
        private string clName2 = "cl14576_2";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14576()
        {
             //1、连接远程sequoiadb，不指定用户名和密码，并创建集合1 
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName1);
            Assert.AreEqual(true, sdb.IsValid());
            //2、连接远程sequoiadb，指定用户名和密码，并创建集合2 ，该场景在seqDB-14567中已覆盖
            //3、连接远程sequoiadb，指定用户名、密码、options参数等，并创建集合3 
            string username = "name14576";
            string passwd = "passwd14576";
            try
            {
                sdb.CreateUser(username, passwd);
                Sequoiadb localdb = new Sequoiadb(SdbTestBase.coordUrl);
                ConfigOptions options = new ConfigOptions();
                options.ConnectTimeout = 12000;
                options.KeepIdle = 50;
                localdb.Connect(username, passwd, options);
                cs = localdb.GetCollectionSpace(SdbTestBase.csName);
                if (cs.IsCollectionExist(clName2))
                {
                    cs.DropCollection(clName2);
                }
                cl = cs.CreateCollection(clName2);
                Assert.AreEqual(true, localdb.IsValid());
                localdb.Disconnect();
                Assert.AreEqual(false, localdb.IsValid());
            }
            catch (BaseException e)
            {
                Assert.Fail(e.Message);
            }
            finally
            {
                sdb.RemoveUser(username, passwd);
            }
            
        }

        [TestCleanup()]
        public void TearDown()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName1))
            {
                cs.DropCollection(clName1);
            }
            if (cs.IsCollectionExist(clName2))
            {
                cs.DropCollection(clName2);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
                Assert.AreEqual(false, sdb.IsValid());
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
