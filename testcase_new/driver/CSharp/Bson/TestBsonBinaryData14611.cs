using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Bson
{

    /**
     * description:  
     *                   ToString()；ToGuid ()；ToGuid (GuidRepresentation guidRepresentation)
     * testcase:         14611
     * author:      chensiqin
     * date:        2019/03/11
    */

    [TestClass]
    public class TestBsonBinaryData14611
    {

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14611()
        {
            byte[] bytes = System.Text.Encoding.Default.GetBytes("hello sequoiadb!");
            byte[] bytes2 = System.Text.Encoding.Default.GetBytes("hello sequoiadb!");
            Guid guid = Guid.NewGuid();
            BsonBinaryData binary1 = new BsonBinaryData(bytes);
            BsonBinaryData binary2 = new BsonBinaryData(bytes, BsonBinarySubType.Binary);
            BsonBinaryData binary3 = new BsonBinaryData(bytes, BsonBinarySubType.UuidLegacy, GuidRepresentation.CSharpLegacy);
            BsonBinaryData binary4 = new BsonBinaryData(bytes2, BsonBinarySubType.UuidLegacy, GuidRepresentation.CSharpLegacy);
            BsonBinaryData binary5 = new BsonBinaryData(guid);
            BsonBinaryData binary6 = new BsonBinaryData(guid, GuidRepresentation.CSharpLegacy);

            string expected1 = "Binary:0x68656c6c6f20736571756f6961646221";
            string expected2 = "UuidLegacy:0x68656c6c6f20736571756f6961646221";
            
            Assert.AreEqual(expected1, binary1.ToString());
            Assert.AreEqual(expected1, binary2.ToString());
            Assert.AreEqual(expected2, binary3.ToString());
            Assert.AreEqual(guid, binary5.ToGuid());
            Assert.AreEqual(guid, binary6.ToGuid());
            Assert.AreEqual(binary3.ToGuid(), binary4.ToGuid());
            Assert.AreEqual(binary3.ToGuid(), binary3.ToGuid(GuidRepresentation.CSharpLegacy));
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
