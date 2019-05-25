package org.springframework.data.mongodb.assist;

/**
 * Created by tanzhaobo on 2017/9/1.
 */

import org.bson.util.annotations.Immutable;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;

/**
 * mongo server address
 */
@Immutable
public class ServerAddress {

    /**
     * Creates a ServerAddress with default host and port
     * @throws UnknownHostException
     */
    public ServerAddress()
            throws UnknownHostException {
        this( defaultHost() , defaultPort() );
    }

    /**
     * Creates a ServerAddress with default port
     * @param host hostname
     * @throws UnknownHostException
     */
    public ServerAddress( String host )
            throws UnknownHostException {
        this( host , defaultPort() );
    }

    /**
     * Creates a ServerAddress
     * @param host hostname
     * @param port mongod port
     * @throws UnknownHostException
     */
    public ServerAddress( String host , int port )
            throws UnknownHostException {
        if ( host == null )
            host = defaultHost();
        host = host.trim();
        if ( host.length() == 0 )
            host = defaultHost();

        if ( host.startsWith( "[" ) ) {
            int idx = host.indexOf( "]" );
            if( idx == -1 )
                throw new IllegalArgumentException( "an IPV6 address must be encosed with '[' and ']'"
                        + " according to RFC 2732." );

            int portIdx = host.indexOf( "]:" );
            if(portIdx != -1) {
                if (port != defaultPort())
                    throw new IllegalArgumentException( "can't specify port in construct and via host" );
                port = Integer.parseInt( host.substring( portIdx + 2 ) );
            }
            host = host.substring(1, idx);
        }
        else {
            int idx = host.indexOf( ":" );
            if ( idx > 0 ){
                if ( port != defaultPort() )
                    throw new IllegalArgumentException( "can't specify port in construct and via host" );
                port = Integer.parseInt( host.substring( idx + 1 ) );
                host = host.substring( 0 , idx ).trim();
            }
        }

        _host = host;
        _port = port;
    }

    /**
     * Creates a ServerAddress with default port
     * @param addr host address
     */
    public ServerAddress( InetAddress addr ){
        this( new InetSocketAddress( addr , defaultPort() ) );
    }

    /**
     * Creates a ServerAddress
     * @param addr host address
     * @param port mongod port
     */
    public ServerAddress( InetAddress addr , int port ){
        this( new InetSocketAddress( addr , port ) );
    }

    /**
     * Creates a ServerAddress
     * @param addr inet socket address containing hostname and port
     */
    public ServerAddress( InetSocketAddress addr ){
        _host = addr.getHostName();
        _port = addr.getPort();
    }



    /**
     * Determines whether this address is the same as a given host.
     * @param host the address to compare
     * @return if they are the same
     */
    public boolean sameHost( String host ){
        int idx = host.indexOf( ":" );
        int port = defaultPort();
        if ( idx > 0 ){
            port = Integer.parseInt( host.substring( idx + 1 ) );
            host = host.substring( 0 , idx );
        }

        return
                _port == port &&
                        _host.equalsIgnoreCase( host );
    }

    @Override
    public boolean equals(final Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        final ServerAddress that = (ServerAddress) o;

        if (_port != that._port) return false;
        if (!_host.equals(that._host)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = _host.hashCode();
        result = 31 * result + _port;
        return result;
    }

    /**
     * Gets the hostname
     * @return hostname
     */
    public String getHost(){
        return _host;
    }

    /**
     * Gets the port number
     * @return port
     */
    public int getPort(){
        return _port;
    }

    /**
     * Gets the underlying socket address
     * @return socket address
     * @throws MongoException.Network if the host can not be resolved
     */
    public InetSocketAddress getSocketAddress() throws UnknownHostException {
        return new InetSocketAddress(InetAddress.getByName(_host), _port);
    }

    @Override
    public String toString(){
        return _host + ":" + _port;
    }

    final String _host;
    final int _port;


    /**
     * Returns the default database host: "127.0.0.1"
     * @return IP address of default host.
     */
    public static String defaultHost(){
        return "127.0.0.1";
    }

    /** Returns the default database port: 11810
     * @return the default port
     */
    public static int defaultPort(){
        return 11810;
    }
}

