package com.sequoiadb.example;

import java.util.Date;
import java.util.Random;
import java.util.regex.Pattern;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.Binary;
import org.bson.types.ObjectId;
import org.testng.annotations.DataProvider;

public class DocProvider {
    private static Random random = new Random();

    @DataProvider(name = "create")
    public static Object[][] createDoc() {
        return new Object[][] { new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) },
                new Object[] { getDoc( false ) } };
    }

    @SuppressWarnings("deprecation")
    public static Object getDoc( boolean recursion ) {
        BSONObject doc = new BasicBSONObject();
        // doc.put("_id", random.nextInt(Integer.MAX_VALUE));

        int typeSelector = random.nextInt( 11 );
        switch ( typeSelector ) {
        case 0: // Integer
            doc.put( "typeint", random.nextInt( Integer.MAX_VALUE ) );
            break;
        case 1: // Float
            doc.put( "typefloat",
                    random.nextInt( Integer.MAX_VALUE ) / Float.MAX_VALUE );
            break;
        case 2: // String
            doc.put( "typestring", getRandomStr() );
            break;
        case 3: // Boolean
            doc.put( "typebool", random.nextBoolean() );
            break;
        case 4: // OID
            doc.put( "bypeoid", ObjectId.get() );
            break;
        case 5: // date
            doc.put( "typedate", new Date( 1900 + random.nextInt( 200 ),
                    random.nextInt( 12 ), random.nextInt( 29 ) ) );
            break;
        case 6: // timestamp
            doc.put( "typetimestamp",
                    new BSONTimestamp( random.nextInt( Integer.MAX_VALUE ),
                            random.nextInt( 1000000 ) ) );
            break;
        case 7: // 二进制
            byte[] arr = getRandomStr().getBytes();
            Binary bindata = new Binary( arr );
            doc.put( "typebinary", bindata );
            break;
        case 8: // 正则
            Pattern obj = Pattern.compile( "^" + random.nextInt( 2016 ),
                    Pattern.CASE_INSENSITIVE );
            doc.put( "typereg", obj );
            break;
        case 9: // 对象
            if ( !recursion ) {
                doc.put( "typeobj", getDoc( true ) );
                break;
            }
        case 10: // 数组
            if ( !recursion ) {
                BasicBSONList docs = new BasicBSONList();
                int arrSize = random.nextInt( 10 );
                for ( int i = 0; i < arrSize; ++i ) {
                    docs.put( Integer.toString( i ), getDoc( true ) );
                }
                doc.put( "typearr", docs );
                break;
            }
        default:
            doc.put( "typelong", random.nextLong() );
            break;
        }

        return doc;
    }

    public static String getRandomStr() {
        final char[] arrChar = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8',
                '9', '&', '|', '{', '}', '[', ']', ',', '湘', '嶴', '応', '冀', '琼',
                '黔', '蜀', '鄂', '陇', '豫', '粤' };

        StringBuffer buff = new StringBuffer();
        int len = 1 * 1024 * 1024; // 2M
        int strLen = random.nextInt( len );
        for ( int i = 0; i < strLen; ++i ) {
            buff.append( arrChar[ random.nextInt( arrChar.length ) ] );
        }
        return buff.toString();
    }
}
