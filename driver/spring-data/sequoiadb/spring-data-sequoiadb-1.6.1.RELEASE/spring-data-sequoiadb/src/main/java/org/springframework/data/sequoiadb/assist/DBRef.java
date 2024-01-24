package org.springframework.data.sequoiadb.assist;

// DBRef.java

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

/**
 * Extends DBRefBase to understand a BSONObject representation of a reference.
 * <p>
 * While instances of this class are {@code Serializable}, deserialized instances can not be fetched,
 * as the {@code db} property is transient.
 *
 * @dochub dbrefs
 */
public class DBRef extends DBRefBase {

    private static final long serialVersionUID = -849581217713362618L;

    /**
     * Creates a DBRef
     * @param db the database
     * @param o a BSON object representing the reference
     */
    public DBRef(DB db , BSONObject o ){
        super( db , o.get( "$ref" ).toString() , o.get( "$id" ) );
    }

    /**
     * Creates a DBRef
     * @param db the database
     * @param ns the namespace where the object is stored
     * @param id the object id
     */
    public DBRef(DB db , String ns , Object id) {
        super(db, ns, id);
    }

    // Required for serialization framework
    private DBRef() {
        super();
    }

    /**
     * fetches a referenced object from the database
     * @param db the database
     * @param ref the reference
     * @return the referenced document
     * @throws BaseException
     */
    public static BSONObject fetch(DB db, BSONObject ref) {
        String ns;
        Object id;

        if ((ns = (String)ref.get("$ref")) != null && (id = ref.get("$id")) != null) {
            return db.getCollection(ns).findOne(new BasicBSONObject("_id", id));
        }
        return null;
    }
}
