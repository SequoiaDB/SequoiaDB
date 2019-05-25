package org.springframework.data.sequoiadb.assist;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;

/**
 * utility for building complex objects
 * example:
 *  BasicBSONObjectBuilder.start().add( "name" , "eliot" ).add( "number" , 17 ).get()
 */
public class BasicBSONObjectBuilder {

    /**
     * creates an empty object
     */
    public BasicBSONObjectBuilder(){
        _stack = new LinkedList<BSONObject>();
        _stack.add( new BasicBSONObject() );
    }

    /**
     * Creates an empty object
     * @return The new empty builder
     */
    public static BasicBSONObjectBuilder start(){
        return new BasicBSONObjectBuilder();
    }

    /**
     * creates an object with the given key/value
     * @param k The field name
     * @param val The value
     */
    public static BasicBSONObjectBuilder start(String k , Object val ){
        return (new BasicBSONObjectBuilder()).add( k , val );
    }

    /**
     * Creates an object builder from an existing map.
     * @param m map to use
     * @return the new builder
     */
    @SuppressWarnings("unchecked")
    public static BasicBSONObjectBuilder start(Map m){
        BasicBSONObjectBuilder b = new BasicBSONObjectBuilder();
        Iterator<Map.Entry> i = m.entrySet().iterator();
        while (i.hasNext()) {
            Map.Entry entry = i.next();
            b.add(entry.getKey().toString(), entry.getValue());
        }
        return b;
    }

    /**
     * appends the key/value to the active object
     * @param key
     * @param val
     * @return returns itself so you can chain
     */
    public BasicBSONObjectBuilder append(String key , Object val ){
        _cur().put( key , val );
        return this;
    }


    /**
     * same as appends
     * @see #append(String, Object)
     * @param key
     * @param val
     * @return returns itself so you can chain
     */
    public BasicBSONObjectBuilder add(String key , Object val ){
        return append( key, val );
    }

    /**
     * creates an new empty object and inserts it into the current object with the given key.
     * The new child object becomes the active one.
     * @param key
     * @return returns itself so you can chain
     */
    public BasicBSONObjectBuilder push(String key ){
        BasicBSONObject o = new BasicBSONObject();
        _cur().put( key , o );
        _stack.addLast( o );
        return this;
    }

    /**
     * pops the active object, which means that the parent object becomes active
     * @return returns itself so you can chain
     */
    public BasicBSONObjectBuilder pop(){
        if ( _stack.size() <= 1 )
            throw new IllegalArgumentException( "can't pop last element" );
        _stack.removeLast();
        return this;
    }

    /**
     * gets the base object
     * @return The base object
     */
    public BSONObject get(){
        return _stack.getFirst();
    }

    /**
     * returns true if no key/value was inserted into base object
     * @return True if empty
     */
    public boolean isEmpty(){
        return ((BasicBSONObject) _stack.getFirst()).size() == 0;
    }

    private BSONObject _cur(){
        return _stack.getLast();
    }

    private final LinkedList<BSONObject> _stack;

}
