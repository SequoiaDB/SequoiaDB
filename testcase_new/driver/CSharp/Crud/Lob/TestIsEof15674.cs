using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;


namespace CSharp.Crud.Lob
{
    /**
    * description: isEof                
    * testcase:    15674
    * author:      csq
    * date:        2018/08/29
    */

    [TestClass]
    public class TestIsEof15674
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl15674";
        private Random random = new Random();
        byte[] testLobBuff = null;
        private ObjectId oid;

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
        public void Test15674()
        {
            putLob();
            readLob();
        }

   public void readLob(){     
        Sequoiadb db2 = new Sequoiadb(SdbTestBase.coordUrl);
        db2.Connect();
       try
        {  
          DBCollection cl2 = db2.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
          DBLob rLob = null;
         rLob = cl2.OpenLob(oid);         
         byte[] rbuff = new byte[1024];
         int readLen =0;          

            byte[] bytebuff = new byte[0];
         
         //flag is true when read operation not completed
         int offset = 0 ;
         while ((readLen = rLob.Read(rbuff)) != -1){
                bytebuff = LobUtils.AppendBuff(bytebuff, rbuff, offset);
                offset = offset + readLen;
                int cnt = testLobBuff.Length - offset;
                if (cnt > 0 && cnt < 1024)
                {
                    rbuff = new byte[cnt];
                }
                
         }
         rLob.Close();
         if(rLob.IsEof()){
                Console.WriteLine(bytebuff.Length);
                Console.WriteLine(testLobBuff.Length);
                LobUtils.AssertByteArrayEqual(bytebuff, testLobBuff);
         }else{
            Assert.Fail("implement isEof() failed when already finish read");
         }
       }
        catch(BaseException e){
            if (-4 != e.ErrorCode && -317 != e.ErrorCode && -268 != e.ErrorCode && -269 != e.ErrorCode)
            {
                Assert.Fail("removeLob fail:" + e.Message + e.ErrorCode);
         }   
       }      
   }

        private void putLob()
        {
            int lobsize = random.Next(1048576);
            testLobBuff = LobUtils.GetRandomBytes(lobsize);
            DBLob lob = null;
            try
            {
                lob = cl.CreateLob();
                lob.Write(testLobBuff);
                oid = lob.GetID();
            }
            catch (BaseException e)
            {
                Assert.Fail("write lob fail"+e.Message);
            }
            finally
            {
                if (lob != null)
                {
                    lob.Close();
                }
            }
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
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }
    }
}
