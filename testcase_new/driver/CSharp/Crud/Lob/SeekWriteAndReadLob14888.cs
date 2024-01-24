using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Lob
{
    /**
    * description:   seek write /seek read/list /remove lob
    *                  test interface:  test the interface write (byte[] b, int off, int len)，read (byte[] b, int off, int len)
    * testcase:      14888
    * author:        wuyan
    * date:          2018/3/16
    */
    [TestClass]
    public class SeekWriteAndReadLob14888
    {
        private const string clName = "writeLob14888";
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;       

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
        public void TestLob14888()
        {
            int writeBuffSize = 1024 * 1024;            
            byte[] writeBuff = LobUtils.GetRandomBytes(writeBuffSize);
            //test write (byte[] b, int off, int len)
            ObjectId oid = SeekWriteLob(writeBuff);
            //test read (byte[] b, int off, int len)
            SeekReadLob(oid, writeBuff);

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

        private ObjectId SeekWriteLob(byte[] writeBuff)
        {            
            int offset = 0;
            int writeLen = 0;
            int totalLen = writeBuff.Length;
            DBLob lob = cl.CreateLob();
            while (totalLen > 0)
            {
                //512k bytes per write
                writeLen = totalLen > 512 ? 512 : totalLen;
                lob.Write(writeBuff, offset, writeLen);
                // back offset write lob
                offset += writeLen;
                totalLen -= writeLen;
            }            
            ObjectId oid = lob.GetID();
            lob.Close();

            //check the all write lob     
            DBLob rlob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
            byte[] readLobBuff = new byte[(int)lob.GetSize()];
            rlob.Read(readLobBuff);
            rlob.Close();
            LobUtils.AssertByteArrayEqual(readLobBuff, writeBuff);
            
            return oid;
        }

        

        private void SeekReadLob(ObjectId oid, byte[] expBuffLob)
        {            
            //read lob and check the write lob result
            DBLob rlob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
            byte[] readLobBuff = new byte[(int)rlob.GetSize()];            
            int readLen = 0;
            int off = 0;
            int perReadLen = 0;
            int totalLen = readLobBuff.Length;
            do
            {
                //512k bytes per read
                perReadLen = totalLen > 512 ? 512 : totalLen;
                //test the return value
                readLen = rlob.Read(readLobBuff, off, perReadLen);
                off += readLen;
                totalLen -= readLen;
            } while (readLen > 0 && totalLen > 0);           
            rlob.Close();
            LobUtils.AssertByteArrayEqual(readLobBuff, expBuffLob);
        }

        private void removeLob(ObjectId oid, byte[] writeBuff)
        {
            DBLob lob = cl.CreateLob();
            lob.Write(writeBuff);
            ObjectId wOid = lob.GetID();
            long size = lob.GetSize();
            lob.Close();
        }

       
    }        
}
