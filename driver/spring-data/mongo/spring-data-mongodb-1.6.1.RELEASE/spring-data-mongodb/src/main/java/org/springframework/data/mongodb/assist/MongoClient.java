package org.springframework.data.mongodb.assist;

import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.List;

/**
 * A MongoDB client with internal connection pooling. For most applications, you should have one MongoClient instance
 * for the entire JVM.
 */
public class MongoClient extends Mongo {

    private final MongoOptions options;

    /**
     * Specified the address of coords.
     *
     * @param coords the coord node list
     * @param options the options for creating connections
     * @param userName the authentication user
     * @param password the password of the authentication user
     * @throws MongoException
     */
    public MongoClient(List<ServerAddress> coords, String userName, String password, MongoOptions options) {
        super(coords, userName, password, options);
        this.options = options;
    }

    public MongoClient() throws UnknownHostException {
        this(DEFAULT_HOST, DEFAULT_PORT);
    }

    /**
     * Creates a Mongo instance based on a (single) mongodb node.
     *
     * @param host the database's host address
     * @param port the port on which the database is running
     * @throws UnknownHostException if the database host cannot be resolved
     * @throws MongoException
     */
    public MongoClient(String host, int port) throws UnknownHostException {
        this(Arrays.asList(new ServerAddress(host, port)), "", "",
                new MongoOptions.Builder().build());
    }

    /**
     * Creates a Mongo instance based on a (single) mongo node using a given ServerAddress and default options.
     *
     * @param addr    the database address
     * @param options default options
     * @throws MongoException
     * @see com.mongodb.ServerAddress
     */
    public MongoClient(ServerAddress addr, MongoOptions options) {
        this(Arrays.asList(addr), "", "", options);
    }

    /**
     * Specified the address of coords.
     *
     * @param coords the coord node list
     * @throws MongoException
     * @see com.mongodb.ServerAddress
     */
    public MongoClient(List<ServerAddress> coords) {
        this(coords, "", "", new MongoOptions.Builder().build());
    }

    public MongoOptions getMongoClientOptions() {
        return options;
    }
}
