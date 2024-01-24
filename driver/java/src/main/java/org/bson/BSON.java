// BSON.java

/**
 * Copyright (C) 2008 10gen Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.bson;

import com.sequoiadb.base.ClientOptions;
import org.bson.types.*;
import org.bson.util.ClassMap;

import java.math.BigDecimal;
import java.nio.charset.Charset;
import java.util.*;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.logging.Logger;
import java.util.regex.Pattern;

/**
 * Global setting for BSON usage.
 */
public class BSON {

    static final Logger LOGGER = Logger.getLogger("org.bson.BSON");

    // ---- basics ----

    public static final byte EOO = 0;
    public static final byte NUMBER = 1;
    public static final byte STRING = 2;
    public static final byte OBJECT = 3;
    public static final byte ARRAY = 4;
    public static final byte BINARY = 5;
    public static final byte UNDEFINED = 6;
    public static final byte OID = 7;
    public static final byte BOOLEAN = 8;
    public static final byte DATE = 9;
    public static final byte NULL = 10;
    public static final byte REGEX = 11;
    public static final byte REF = 12;
    public static final byte CODE = 13;
    public static final byte SYMBOL = 14;
    public static final byte CODE_W_SCOPE = 15;
    public static final byte NUMBER_INT = 16;
    public static final byte TIMESTAMP = 17;
    public static final byte NUMBER_LONG = 18;

    public static final byte NUMBER_DECIMAL = 100;

    public static final byte MINKEY = -1;
    public static final byte MAXKEY = 127;

    // --- binary types
    /*
       these are binary types
       so the format would look like
       <BINARY><name><BINARY_TYPE><...>
    */

    public static final byte B_GENERAL = 0;
    public static final byte B_FUNC = 1;
    public static final byte B_BINARY = 2;
    public static final byte B_UUID = 3;

    // ---- regular expression handling ----

    /** Converts a string of regular expression flags from the database in Java regular
     * expression flags.
     * @param flags flags from database
     * @return the Java flags
     */
    public static int regexFlags(String flags) {
        int fint = 0;
        if (flags == null || flags.length() == 0)
            return fint;

        flags = flags.toLowerCase();

        for (int i = 0; i < flags.length(); i++) {
            RegexFlag flag = RegexFlag.getByCharacter(flags.charAt(i));
            if (flag != null) {
                fint |= flag.javaFlag;
                if (flag.unsupported != null)
                    _warnUnsupportedRegex(flag.unsupported);
            } else {
                throw new IllegalArgumentException("unrecognized flag [" + flags.charAt(i) + "] " + (int) flags.charAt(i));
            }
        }
        return fint;
    }

    public static int regexFlag(char c) {
        RegexFlag flag = RegexFlag.getByCharacter(c);
        if (flag == null)
            throw new IllegalArgumentException("unrecognized flag [" + c + "]");

        if (flag.unsupported != null) {
            _warnUnsupportedRegex(flag.unsupported);
            return 0;
        }

        return flag.javaFlag;
    }

    /** Converts Java regular expression flags into a string of flags for the database
     * @param flags Java flags
     * @return the flags for the database
     */
    public static String regexFlags(int flags) {
        StringBuilder buf = new StringBuilder();

        for (RegexFlag flag : RegexFlag.values()) {
            if ((flags & flag.javaFlag) > 0) {
                buf.append(flag.flagChar);
                flags -= flag.javaFlag;
            }
        }

        if (flags > 0)
            throw new IllegalArgumentException("some flags could not be recognized.");

        return buf.toString();
    }

    private static enum RegexFlag {
        CANON_EQ(Pattern.CANON_EQ, 'c', "Pattern.CANON_EQ"),
        UNIX_LINES(Pattern.UNIX_LINES, 'd', "Pattern.UNIX_LINES"),
        GLOBAL(GLOBAL_FLAG, 'g', null),
        CASE_INSENSITIVE(Pattern.CASE_INSENSITIVE, 'i', null),
        MULTILINE(Pattern.MULTILINE, 'm', null),
        DOTALL(Pattern.DOTALL, 's', "Pattern.DOTALL"),
        LITERAL(Pattern.LITERAL, 't', "Pattern.LITERAL"),
        UNICODE_CASE(Pattern.UNICODE_CASE, 'u', "Pattern.UNICODE_CASE"),
        COMMENTS(Pattern.COMMENTS, 'x', null);

        private static final Map<Character, RegexFlag> byCharacter = new HashMap<Character, RegexFlag>();

        static {
            for (RegexFlag flag : values()) {
                byCharacter.put(flag.flagChar, flag);
            }
        }

        public static RegexFlag getByCharacter(char ch) {
            return byCharacter.get(ch);
        }

        public final int javaFlag;
        public final char flagChar;
        public final String unsupported;

        RegexFlag(int f, char ch, String u) {
            javaFlag = f;
            flagChar = ch;
            unsupported = u;
        }
    }

    private static void _warnUnsupportedRegex(String flag) {
        LOGGER.info("flag " + flag + " not supported by db.");
    }

    private static final int GLOBAL_FLAG = 256;

    // --- (en|de)coding hooks -----

    /**
     *  check has decode hooks or not.
     * @return true for having, and false for not
     */
    public static boolean hasDecodeHooks() {
        return _decodeHooks;
    }

    /**
     *  Add a encoding hook for transforming a instance of class c into a
     *  instance of the BSON supported classes.
     * <pre>
     *  Usage:
     *  public void testEncodeDecodeHook() {
     *       BSON.addEncodingHook(GregorianCalendar.class, new Transformer() {
     *          public Object transform(Object o) {
     *               return ((GregorianCalendar)o).getTime();
     *           }
     *       });
     *
     *       BSON.addDecodingHook(Date.class, new Transformer() {
     *           public Object transform(Object o) {
     *               Calendar calendar = new GregorianCalendar();
     *               calendar.setTime((Date)o);
     *               return calendar;
     *           }
     *       });
     *
     *       Calendar calendar = new GregorianCalendar();
     *       calendar.set(2015, 0,1);
     *       BSONObject object = new BasicBSONObject();
     *       object.put("calendar", calendar);
     *       cl.insert(object);
     *       DBCursor cursor = cl.query();
     *       while(cursor.hasNext()) {
     *           BSONObject record = cursor.getNext();
     *           System.out.println("record is: " + record);
     *           Object resultObject = record.get("calendar");
     *           calendar.equals(resultObject);
     *           Assert.assertTrue(resultObject instanceof GregorianCalendar);
     *       }
     *   }
     * </pre>
     * @param c the class of the instance which is going to be transformed.
     * @param t the Transformer, user should defines the transform rule.
     */
    public static void addEncodingHook(Class c, Transformer t) {
        _encodeHooks = true;
        List<Transformer> l = _encodingHooks.get(c);
        if (l == null) {
            l = new CopyOnWriteArrayList<Transformer>();
            _encodingHooks.put(c, l);
        }
        l.add(t);
    }

    /**
     *  Add a decoding hook for transforming a instance of BSON supported classes into a
     *  instance of class c.
     * <pre>
     *  Usage:
     *  public void testEncodeDecodeHook() {
     *       BSON.addEncodingHook(GregorianCalendar.class, new Transformer() {
     *          public Object transform(Object o) {
     *               return ((GregorianCalendar)o).getTime();
     *           }
     *       });
     *
     *       BSON.addDecodingHook(Date.class, new Transformer() {
     *           public Object transform(Object o) {
     *               Calendar calendar = new GregorianCalendar();
     *               calendar.setTime((Date)o);
     *               return calendar;
     *           }
     *       });
     *
     *       Calendar calendar = new GregorianCalendar();
     *       calendar.set(2015, 0,1);
     *       BSONObject object = new BasicBSONObject();
     *       object.put("calendar", calendar);
     *       cl.insert(object);
     *       DBCursor cursor = cl.query();
     *       while(cursor.hasNext()) {
     *           BSONObject record = cursor.getNext();
     *           System.out.println("record is: " + record);
     *           Object resultObject = record.get("calendar");
     *           calendar.equals(resultObject);
     *           Assert.assertTrue(resultObject instanceof GregorianCalendar);
     *       }
     *   }
     * </pre>
     * @param c the class of the instance which is going to be transformed into.
     * @param t the Transformer, user should defines the transform rule.
     */
    public static void addDecodingHook(Class c, Transformer t) {
        _decodeHooks = true;
        List<Transformer> l = _decodingHooks.get(c);
        if (l == null) {
            l = new CopyOnWriteArrayList<Transformer>();
            _decodingHooks.put(c, l);
        }
        l.add(t);
    }


    /**
     *  Use the registered encoding hooks to transform the specific instance into the instance of BSON
     *  supported classes.
     * @param o
     * @return
     */
    public static Object applyEncodingHooks(Object o) {
        if (!_anyHooks())
            return o;

        if (_encodingHooks.size() == 0 || o == null)
            return o;
        List<Transformer> l = _encodingHooks.get(o.getClass());
        if (l != null)
            for (Transformer t : l)
                o = t.transform(o);
        return o;
    }

    /**
     *  Use the registered decoding hooks to transform the instance of BSON supported classes into another
     *  instance of user expect.
     * @param o
     * @return
     */
    public static Object applyDecodingHooks(Object o) {
        if (!_anyHooks() || o == null)
            return o;

        List<Transformer> l = _decodingHooks.get(o.getClass());
        if (l != null)
            for (Transformer t : l)
                o = t.transform(o);
        return o;
    }

    /**
     *  Returns the encoding hook(s) associated with the specified class
     * @param c the specified class for getting it's registered encoding hook.
     * @return all the registered encoding hooks.
     */
    public static List<Transformer> getEncodingHooks(Class c) {
        return _encodingHooks.get(c);
    }

    /**
     * Clears all encoding hooks.
     */
    public static void clearEncodingHooks() {
        _encodeHooks = false;
        _encodingHooks.clear();
    }

    /**
     * Remove all encoding hooks for a specific class.
     */
    public static void removeEncodingHooks(Class c) {
        _encodingHooks.remove(c);
    }

    /**
     * Remove a specific encoding hook for a specific class.
     */
    public static void removeEncodingHook(Class c, Transformer t) {
        getEncodingHooks(c).remove(t);
    }

    /**
     * Returns the decoding hook(s) associated with the specific class
     */
    public static List<Transformer> getDecodingHooks(Class c) {
        return _decodingHooks.get(c);
    }

    /**
     * Clears all decoding hooks.
     */
    public static void clearDecodingHooks() {
        _decodeHooks = false;
        _decodingHooks.clear();
    }

    /**
     * Remove all decoding hooks for a specific class.
     */
    public static void removeDecodingHooks(Class c) {
        _decodingHooks.remove(c);
    }

    /**
     * Remove a specific encoding hook for a specific class.
     */
    public static void removeDecodingHook(Class c, Transformer t) {
        getDecodingHooks(c).remove(t);
    }


    /**
     * Remove all the hooks.
     */
    public static void clearAllHooks() {
        clearEncodingHooks();
        clearDecodingHooks();
    }

    /**
     * Returns true if any encoding or decoding hooks are loaded.
     */
    private static boolean _anyHooks() {
        return _encodeHooks || _decodeHooks;
    }

    private static boolean _encodeHooks = false;
    private static boolean _decodeHooks = false;
    static ClassMap<List<Transformer>> _encodingHooks =
            new ClassMap<List<Transformer>>();

    static ClassMap<List<Transformer>> _decodingHooks =
            new ClassMap<List<Transformer>>();

    static protected Charset _utf8 = Charset.forName("UTF-8");

    // ----- static encode/_decode -----

    /**
     *  Encoding a BSONObject instance into bytes
     * @param o the instance of BSONObject.
     * @return the bytes.
     */
    public static byte[] encode(BSONObject o) {
        return encode(o, null);
    }

    // Internal use
    public static byte[] encode(BSONObject o, BSONObject extendObj) {
        BSONEncoder e = _staticEncoder.get();
        try {
            return e.encode(o, extendObj);
        } finally {
            e.done();
        }
    }

    /**
     *  Decode the bytes into a BSONObject instance.
     * @param b bytes to be decoded.
     * @return a BSONObject instance.
     */
    public static BSONObject decode(byte[] b) {
        BSONDecoder d = _staticDecoder.get();
        return d.readObject(b);
    }

    /**
     *  Decode the bytes into a BSONObject instance.
     * @param b bytes to be decoded.
     * @param offset the start position.
     * @return a BSONObject instance.
     *
     */
    public static BSONObject decode(byte[] b, int offset) {
        BSONDecoder d = _staticDecoder.get();
        return d.readObject(b, offset);
    }

    static ThreadLocal<BSONEncoder> _staticEncoder = new ThreadLocal<BSONEncoder>() {
        protected BSONEncoder initialValue() {
            return new BasicBSONEncoder();
        }
    };

    static ThreadLocal<BSONDecoder> _staticDecoder = new ThreadLocal<BSONDecoder>() {
        protected BSONDecoder initialValue() {
            return new NewBSONDecoder();
        }
    };

    // --- coercing ---

    public static int toInt(Object o) {
        if (o == null)
            throw new NullPointerException("can't be null");

        if (o instanceof Number)
            return ((Number) o).intValue();

        if (o instanceof Boolean)
            return ((Boolean) o) ? 1 : 0;

        throw new IllegalArgumentException("can't convert: " + o.getClass().getName() + " to int");
    }

    public static boolean IsBasicType(Object obj) {
        if (obj == null)
            return true;
        else if (obj.getClass().isPrimitive())
            return true;
        else if (obj instanceof BSONDate)
            return true;
        else if (obj instanceof Date)
            return true;
        else if (obj instanceof Number)
            return true;
        else if (obj instanceof Character)
            return true;
        else if (obj instanceof String)
            return true;
        else if (obj instanceof ObjectId)
            return true;
            // else if (obj instanceof BSONObject)
            // return true;
        else if (obj instanceof Boolean)
            return true;
        else if (obj instanceof Pattern)
            return true;
            // else if (obj instanceof Map)
            // return true;
            // else if (obj instanceof Iterable)
            // return true;
        else if (obj instanceof byte[])
            return true;
        else if (obj instanceof Binary)
            return true;
        else if (obj instanceof UUID)
            return true;
            // else if (obj.getClass().isArray())
            // return true;
        else if (obj instanceof Symbol)
            return true;
        else if (obj instanceof BSONTimestamp)
            return true;
        else if (obj instanceof BSONDecimal)
            return true;
        else if (obj instanceof BigDecimal)
            return true;
        else if (obj instanceof CodeWScope)
            return true;
        else if (obj instanceof Code)
            return true;
        else if (obj instanceof MinKey)
            return true;
        else if (obj instanceof MaxKey)
            return true;
        else
            return false;
    }

    // setting display mode
    private static boolean _compatible = false;

    /**
     * When "compatible" is true, the content of BasicBSONObject method "toString" is show
     * absolutely the same with which is show in sdb shell.
     * @param compatible true or false, default to be false;
     *
     * {@code
     *  // we have a bson as below:
     *  BSONObject obj = new BasicBSONObject("a", Long.MAX_VALUE);
     *  // sdb shell shows this bson like this:
     *  {"a" : { "$numberLong" : "9223372036854775807"}}
     *  // sdb shell use javascript grammer, so, it can't display number
     *  // which is great that 2^53 - 1. So it use "$numberLong" to represent
     *  // the type, and keep the number between the quotes.
     *  // However, in java, when we use "obj.toString()",
     *  // most of the time, we don't hope to get a result with
     *  // the format "$numberLong", we hope to see the result as
     *  // below:
     *  {"a" : 9223372036854775807}
     *  // When parameter "compatible" is false, we get this kind of result
     *  // all the time. Otherwise, we get a result which is show as the sdb shell shows.
     * }
     */
    public static void setJSCompatibility(boolean compatible) {
        _compatible = compatible;
    }

    /**
     * Get whether the display mode of BSON is the same with that in sdb shell or not.
     * @return true or false.
     * @see #setJSCompatibility(boolean)
     */
    public static boolean getJSCompatibility() {
        return _compatible;
    }

    // Whether to use exactly date, default is false.
    // True:  only the year, month and day parts of java.util.Date are retained in BSON.encode()
    // False: java.util.Date remains intact in BSON.encode()
    private static boolean exactlyDate = false;

    public static void setExactlyDate( boolean value) {
        exactlyDate = value;
    }

    public static boolean getExactlyDate() {
        return exactlyDate;
    }
}
