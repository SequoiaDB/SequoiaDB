
/**
 *      Copyright (C) 2008 10gen Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

package org.bson.types;

import java.net.*;
import java.nio.*;
import java.util.*;
import java.util.concurrent.atomic.*;
import java.util.logging.*;

/**
 * A globally unique identifier for objects.
 * <p>Consists of 12 bytes, divided as follows:
 * <blockquote><pre>
 * <table border="1">
 * <tr><td>0</td><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td><td>6</td>
 *     <td>7</td><td>8</td><td>9</td><td>10</td><td>11</td></tr>
 * <tr><td colspan="4">time</td><td colspan="3">machine</td>
 *     <td colspan="2">pid</td><td colspan="3">inc</td></tr>
 * </table>
 * </pre></blockquote>
 *
 */
public class ObjectId implements Comparable<ObjectId> , java.io.Serializable {

    private static final long serialVersionUID = -4415279469780082174L;

    static final Logger LOGGER = Logger.getLogger( "org.bson.ObjectId" );

    /** Gets a new object id.
     * @return the new id
     */
    public static ObjectId get(){
        return new ObjectId();
    }

    /** Checks if a string could be an <code>ObjectId</code>.
     * @return whether the string could be an object id
     */
    public static boolean isValid( String s ){
        if ( s == null )
            return false;

        final int len = s.length();
        if ( len != 24 )
            return false;

        for ( int i=0; i<len; i++ ){
            char c = s.charAt( i );
            if ( c >= '0' && c <= '9' )
                continue;
            if ( c >= 'a' && c <= 'f' )
                continue;
            if ( c >= 'A' && c <= 'F' )
                continue;

            return false;
        }

        return true;
    }

    /** Turn an object into an <code>ObjectId</code>, if possible.
     * Strings will be converted into <code>ObjectId</code>s, if possible, and <code>ObjectId</code>s will
     * be cast and returned.  Passing in <code>null</code> returns <code>null</code>.
     * @param o the object to convert
     * @return an <code>ObjectId</code> if it can be massaged, null otherwise
     */
    public static ObjectId massageToObjectId( Object o ){
        if ( o == null )
            return null;

        if ( o instanceof ObjectId )
            return (ObjectId)o;

        if ( o instanceof String ){
            String s = o.toString();
            if ( isValid( s ) )
                return new ObjectId( s );
        }

        return null;
    }

    public ObjectId( Date time ){
        this(time, _genmachine, _nextInc.getAndIncrement());
    }

    public ObjectId( Date time , int inc ){
        this( time , _genmachine , inc );
    }

    public ObjectId( Date time , int machine , int inc ){
        _time = (int)(time.getTime() / 1000);
        _machine = machine;
        _inc = inc;
        _new = false;
    }

    /** Creates a new instance from a string.
     * @param s the string to convert
     * @throws IllegalArgumentException if the string is not a valid id
     */
    public ObjectId( String s ){
        this( s , false );
    }

    public ObjectId( String s , boolean babble ){

        if ( ! isValid( s ) )
            throw new IllegalArgumentException( "invalid ObjectId [" + s + "]" );

        if ( babble )
            s = babbleToMongod( s );

        byte b[] = new byte[12];
        for ( int i=0; i<b.length; i++ ){
            b[i] = (byte)Integer.parseInt( s.substring( i*2 , i*2 + 2) , 16 );
        }
        ByteBuffer bb = ByteBuffer.wrap( b );
        _time = bb.getInt();
        _machine = bb.getInt();
        _inc = bb.getInt();
        _new = false;
    }

    public ObjectId( byte[] b ){
        if ( b.length != 12 )
            throw new IllegalArgumentException( "need 12 bytes" );
        ByteBuffer bb = ByteBuffer.wrap( b );
        _time = bb.getInt();
        _machine = bb.getInt();
        _inc = bb.getInt();
        _new = false;
    }

    /**
     * Creates an ObjectId
     * @param time time in seconds
     * @param machine machine ID
     * @param inc incremental value
     */
    public ObjectId( int time , int machine , int inc ){
        _time = time;
        _machine = machine;
        _inc = inc;
        _new = false;
    }

    /** Create a new object id.
     */
    public ObjectId(){
        _time = (int) (System.currentTimeMillis() / 1000);
        _machine = _genmachine;
        _inc = _nextInc.getAndIncrement();
        _new = true;
    }

    public int hashCode(){
        int x = _time;
        x += ( _machine * 111 );
        x += ( _inc * 17 );
        return x;
    }

    public boolean equals( Object o ){

        if ( this == o )
            return true;

        ObjectId other = massageToObjectId( o );
        if ( other == null )
            return false;

        return
            _time == other._time &&
            _machine == other._machine &&
            _inc == other._inc;
    }

    public String toStringBabble(){
        return babbleToMongod( toStringMongod() );
    }

    public String toStringMongod(){
        byte b[] = toByteArray();

        StringBuilder buf = new StringBuilder(24);

        for ( int i=0; i<b.length; i++ ){
            int x = b[i] & 0xFF;
            String s = Integer.toHexString( x );
            if ( s.length() == 1 )
                buf.append( "0" );
            buf.append( s );
        }

        return buf.toString();
    }

    public byte[] toByteArray(){
        byte b[] = new byte[12];
        ByteBuffer bb = ByteBuffer.wrap( b );
        bb.putInt( _time );
        bb.putInt( _machine );
        bb.putInt( _inc );
        return b;
    }

    static String _pos( String s , int p ){
        return s.substring( p * 2 , ( p * 2 ) + 2 );
    }

    public static String babbleToMongod( String b ){
        if ( ! isValid( b ) )
            throw new IllegalArgumentException( "invalid object id: " + b );

        StringBuilder buf = new StringBuilder( 24 );
        for ( int i=7; i>=0; i-- )
            buf.append( _pos( b , i ) );
        for ( int i=11; i>=8; i-- )
            buf.append( _pos( b , i ) );

        return buf.toString();
    }

    public String toString(){
        return toStringMongod();
    }

    int _compareUnsigned( int i , int j ){
        long li = 0xFFFFFFFFL;
        li = i & li;
        long lj = 0xFFFFFFFFL;
        lj = j & lj;
        long diff = li - lj;
        if (diff < Integer.MIN_VALUE)
            return Integer.MIN_VALUE;
        if (diff > Integer.MAX_VALUE)
            return Integer.MAX_VALUE;
        return (int) diff;
    }

    public int compareTo( ObjectId id ){
        if ( id == null )
            return -1;

        int x = _compareUnsigned( _time , id._time );
        if ( x != 0 )
            return x;

        x = _compareUnsigned( _machine , id._machine );
        if ( x != 0 )
            return x;

        return _compareUnsigned( _inc , id._inc );
    }

    public int getMachine(){
        return _machine;
    }

    /**
     * Gets the time of this ID, in milliseconds
     */
    public long getTime(){
        return _time * 1000L;
    }

    /**
     * Gets the time of this ID, in seconds
     */
    public int getTimeSecond(){
        return _time;
    }

    public int getInc(){
        return _inc;
    }

    public int _time(){
        return _time;
    }
    public int _machine(){
        return _machine;
    }
    public int _inc(){
        return _inc;
    }

    public boolean isNew(){
        return _new;
    }

    public void notNew(){
        _new = false;
    }

    /**
     * Gets the generated machine ID, identifying the machine / process / class loader
     */
    public static int getGenMachineId() {
        return _genmachine;
    }

    /**
     * Gets the current value of the auto increment
     */
    public static int getCurrentInc() {
        return _nextInc.get();
    }

    final int _time;
    final int _machine;
    final int _inc;

    boolean _new;

    public static int _flip( int x ){
        int z = 0;
        z |= ( ( x << 24 ) & 0xFF000000 );
        z |= ( ( x << 8 )  & 0x00FF0000 );
        z |= ( ( x >> 8 )  & 0x0000FF00 );
        z |= ( ( x >> 24 ) & 0x000000FF );
        return z;
    }

    private static AtomicInteger _nextInc = new AtomicInteger( (new java.util.Random()).nextInt() );

    private static final int _genmachine;
    static {

        try {
            int machinePiece;
            {
                try {
                    StringBuilder sb = new StringBuilder();
                    Enumeration<NetworkInterface> e = NetworkInterface.getNetworkInterfaces();
                    while ( e.hasMoreElements() ){
                        NetworkInterface ni = e.nextElement();
                        sb.append( ni.toString() );
                    }
                    machinePiece = sb.toString().hashCode() << 16;
                } catch (Throwable e) {
                    LOGGER.log(Level.WARNING, e.getMessage(), e);
                    machinePiece = (new Random().nextInt()) << 16;
                }
                LOGGER.fine( "machine piece post: " + Integer.toHexString( machinePiece ) );
            }

            final int processPiece;
            {
                int processId = new java.util.Random().nextInt();
                try {
                    processId = java.lang.management.ManagementFactory.getRuntimeMXBean().getName().hashCode();
                }
                catch ( Throwable t ){
                }

                ClassLoader loader = ObjectId.class.getClassLoader();
                int loaderId = loader != null ? System.identityHashCode(loader) : 0;

                StringBuilder sb = new StringBuilder();
                sb.append(Integer.toHexString(processId));
                sb.append(Integer.toHexString(loaderId));
                processPiece = sb.toString().hashCode() & 0xFFFF;
                LOGGER.fine( "process piece: " + Integer.toHexString( processPiece ) );
            }

            _genmachine = machinePiece | processPiece;
            LOGGER.fine( "machine : " + Integer.toHexString( _genmachine ) );
        }
        catch ( Exception e ){
            throw new RuntimeException( e );
        }

    }
}

