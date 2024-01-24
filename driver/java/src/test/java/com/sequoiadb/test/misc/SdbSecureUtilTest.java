package com.sequoiadb.test.misc;

import com.sequoiadb.util.SdbSecureUtil;
import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.*;
import org.junit.*;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class SdbSecureUtilTest {

    @Test
    public void test() {
        // case 1: null
        BSONObject data = null;

        String value = SdbSecureUtil.toSecurityStr(data, true);
        Assert.assertNull(value);

        // case 2: empty BSON
        data = new BasicBSONList();
        value = SdbSecureUtil.toSecurityStr(data, true);
        Assert.assertEquals(data.toString(), value);

        data = new BasicBSONObject();
        value = SdbSecureUtil.toSecurityStr(data, true);
        Assert.assertEquals(data.toString(), value);

        // case 3: disable encryption
        data.put("a", 1);
        value = SdbSecureUtil.toSecurityStr(data, false);
        Assert.assertEquals(data.toString(), value);
    }

    @Test
    public void BsonTest() throws Exception {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd-HH.mm.ss.SSSSSS");
        Date date = format.parse("2023-09-18-17.04.01.123456");
        BSONDate bsonDate = new BSONDate(date.getTime());
        BSONTimestamp ts = new BSONTimestamp(date);

        String text = "abcdefghijklmnopqrstuvwxyz" +
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
                "1234567890" +
                "`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/? " +
                "你好" +
                "·！￥……（）——【】；’，。《》？、";
        BSONObject obj = new BasicBSONObject("a", "123");
        List<String> array = new ArrayList<>();
        array.add("a");
        array.add("b");

        Binary binary = new Binary(BSON.B_GENERAL, "Hello, world!".getBytes());
        ObjectId oid = new ObjectId("65080f8be2a65fd2d8d9eb58");

        BSONObject data = new BasicBSONObject();

        data.put("int", 123);
        data.put("long", 12345L);
        data.put("double", 123.456);
        data.put("string", text);
        data.put("object", obj);
        data.put("array", array);
        data.put("binary", binary);
        data.put("oid", oid);
        data.put("true", true);
        data.put("false", false);
        data.put("date", date);
        data.put( "bsonDate", bsonDate );
        data.put("timestamp", ts);
        data.put("decimal", new BSONDecimal("12345678901234567890.09876543210987654321"));
        data.put("null", null);
        data.put("maxKey", new MaxKey());
        data.put("minKey", new MinKey());

        String result = SdbSecureUtil.toSecurityStr(data, true);

        // same result as engine
        String expected = "SDBSECURE0000(hfIciA3/CcI1CNHfSfIwCJQwk+3tCcI1CNHfSmV4CJaoCsDlnAQwMWCoBcIySqSeTNE+CJaoCtT/jszeMfCoBcIcFAQqMOLsM+puisxwkA3ljPGfj5D4ntn9hYuKVrTHDEMPWHzRW/yTXr2VELQXLGLAL4pMAqHfSmV4Tqj9BXKogcGICfVzYcFvRJrxYm/dA5xngLyjgNw1Q4acZNaeUc87CBW2bBAzljR566fK660z9bJs9bJs66fC66fQ9bJE9bJE99JV99JD66fk9bJM66fS99JJ99JR99JZ66fg99JKCcIwCJQlFsuzF5VcCNbohfIcFWCoBcIcSXCmCcK2CJaoCsGfjsG3CcI1CGwoCsHcCJaoCsCcCG/oZJIcFszeFYQ3CcI1CPwoCcDciA3pjtrcCNboCzTPLtTcDmpmWEprnsTxhOxQEX/2CcIwCJCrnPzaMWCoBcIcSJCogWIwCJQliAVcCNbohfIcQO2uMJCoBcIcTqEaBNKsBOQzSsH+TAMrSsV9MNzzFqE9CcK2CJaoCtDfnAEcCNbonPQ4MWIwCJQsFAymMWCoBcKsFAymMWIwCJQrFYDzCcI1CPwoCcDrFYDzCcI1CJCfSNCmZXI3ZXH9CcK2CJaoCsQmk+3HFYDzCcI1CPwoCcDrFYDzCcI1CJCfSNCmZXI3ZXH9CcK2CJaoCtDukALmnOGxjJCoBcK6CJCrnPScCNboSXF3TXIfTmr+TJIwCJCriA3qCcI1CNV4TqIaSJK2CJaoCsDzF+zxFAacCNbohfIcQODzF+zxFAacCNboCqHfSmV4Tqj9BXIySqS/TXF5BNraZqI3BNj+TXVmSqHaBXo5TqE/SmCyCcK2CJaoCs34kOacCNboCO34kOaoCJaoCs4phHxzhWCoBcK6CJCrkAG9W+L3CcI1CNHogWIwCJQxiA3ZMYrcCNbohfIcQO4ukrxzhWCoBcIyCP/ogV==)";

        // check
        Assert.assertEquals(expected, result);
    }
}
