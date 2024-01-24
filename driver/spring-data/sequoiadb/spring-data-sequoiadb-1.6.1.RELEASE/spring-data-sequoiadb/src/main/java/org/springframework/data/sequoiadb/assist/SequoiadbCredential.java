package org.springframework.data.sequoiadb.assist;

import org.bson.util.annotations.Immutable;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Represents credentials to authenticate to a sdb server, as well as the source of the credentials and
 * the authentication mechanism to use.
 *
 * @since 2.11.0
 */
@Immutable
public final class SequoiadbCredential {

    /**
     * The SequoiaDB Challenge Response mechanism.
     */
    public static final String SEQUOIADB_CR_MECHANISM = "SEQUOIADB-CR";

    /**
     * The GSSAPI mechanism.  See the <a href="http://tools.ietf.org/html/rfc4752">RFC</a>.
     *
     * @sequoiadb.server.release 2.4
     */
    public static final String GSSAPI_MECHANISM = "GSSAPI";

    /**
     * The PLAIN mechanism.  See the <a href="http://www.ietf.org/rfc/rfc4616.txt">RFC</a>.
     *
     * @sequoiadb.server.release 2.6
     */
    public static final String PLAIN_MECHANISM = "PLAIN";


    /**
     * The SequoiaDB X.509
     *
     * @sequoiadb.server.release 2.6
     */
    public static final String SEQUOIADB_X509_MECHANISM = "SEQUOIADB-X509";

    private final String mechanism;
    private final String userName;
    private final String source;
    private final char[] password;
    private final Map<String, Object> mechanismProperties;

    /**
     * Creates a SequoiadbCredential instance for the SequoiaDB Challenge Response protocol.
     *
     * @param userName the user name
     * @param database the database where the user is defined
     * @param password the user's password
     * @return the credential
     */
    public static SequoiadbCredential createSequoiadbCRCredential(String userName, String database, char[] password) {
        return new SequoiadbCredential(SEQUOIADB_CR_MECHANISM, userName, database, password);
    }

    /**
     * Creates a SequoiadbCredential instance for the GSSAPI SASL mechanism.  To override the default service name of {@code "sequoiadb"},
     * add a mechanism property with the name {@code "SERVICE_NAME"}. To force canonicalization of the host name prior to authentication,
     * add a mechanism property with the name {@code "CANONICALIZE_HOST_NAME"} with the value{@code true}.
     *
     * @param userName the user name
     * @return the credential
     * @see #withMechanismProperty(String, Object)
     *
     * @sequoiadb.server.release 2.4
     */
    public static SequoiadbCredential createGSSAPICredential(String userName) {
        return new SequoiadbCredential(GSSAPI_MECHANISM, userName, "$external", null);
    }

    /**
     * Creates a SequoiadbCredential instance for the SequoiaDB X.509 protocol.
     *
     * @param userName the user name
     * @return the credential
     *
     * @sequoiadb.server.release 2.6
     */
    public static SequoiadbCredential createSequoiadbX509Credential(String userName) {
        return new SequoiadbCredential(SEQUOIADB_X509_MECHANISM, userName, "$external", null);
    }

    /**
     * Creates a SequoiadbCredential instance for the PLAIN SASL mechanism.
     *
     * @param userName the non-null user name
     * @param source the source where the user is defined.  This can be either {@code "$external"} or the name of a database.
     * @param password the non-null user password
     * @return the credential
     *
     * @sequoiadb.server.release 2.6
     */
    public static SequoiadbCredential createPlainCredential(final String userName, final String source, final char[] password) {
        return new SequoiadbCredential(PLAIN_MECHANISM, userName, source, password);
    }


    /**
     * Creates a new SequoiadbCredential as a copy of this instance, with the specified mechanism property added.
     *
     * @param key the key to the property
     * @param value the value of the property
     * @param <T> the property type
     * @return the credential
     */
    public <T> SequoiadbCredential withMechanismProperty(String key, T value) {
        return new SequoiadbCredential(this, key, value);
    }

    /**
     *
     * Constructs a new instance using the given mechanism, userName, source, and password
     *
     * @param mechanism the authentication mechanism
     * @param userName the user name
     * @param source the source of the user name, typically a database name
     * @param password the password
     */
    SequoiadbCredential(final String mechanism, final String userName, final String source, final char[] password) {
        if (mechanism == null) {
            throw new IllegalArgumentException("mechanism can not be null");
        }

        if (userName == null) {
            throw new IllegalArgumentException("username can not be null");
        }

        if (mechanism.equals(SEQUOIADB_CR_MECHANISM) && password == null) {
            throw new IllegalArgumentException("Password can not be null for " + SEQUOIADB_CR_MECHANISM + " mechanism");
        }

        if (mechanism.equals(GSSAPI_MECHANISM) && password != null) {
            throw new IllegalArgumentException("Password must be null for the " + GSSAPI_MECHANISM + " mechanism");
        }

        this.mechanism = mechanism;
        this.userName = userName;
        this.source = source;
        this.password = password != null ? password.clone() : null;
        this.mechanismProperties = Collections.emptyMap();
    }

    /**
     * Constructs a new instance using the given credential plus an additional mechanism property.
     *
     * @param from the credential to copy from
     * @param mechanismPropertyKey the new mechanism property key
     * @param mechanismPropertyValue the new mechanism property value
     * @param <T> the mechanism property type
     */
    <T> SequoiadbCredential(final SequoiadbCredential from, final String mechanismPropertyKey, final T mechanismPropertyValue) {
        this.mechanism = from.mechanism;
        this.userName = from.userName;
        this.source = from.source;
        this.password = from.password;
        this.mechanismProperties = new HashMap<String, Object>(from.mechanismProperties);
        this.mechanismProperties.put(mechanismPropertyKey, mechanismPropertyValue);
    }

    /**
     * Gets the mechanism
     *
     * @return the mechanism.
     */
    public String getMechanism() {
        return mechanism;
    }

    /**
     * Gets the user name
     *
     * @return the user name.  Can never be null.
     */
    public String getUserName() {
        return userName;
    }

    /**
     * Gets the source of the user name, typically the name of the database where the user is defined.
     *
     * @return the user name.  Can never be null.
     */
    public String getSource() {
        return source;
    }

    /**
     * Gets the password.
     *
     * @return the password.  Can be null for some mechanisms.
     */
    public char[] getPassword() {
        if (password == null) {
            return null;
        }
        return password.clone();
    }


    /**
     * Get the value of the given key to a mechanism property, or defaultValue if there is no mapping.
     *
     * @param key the mechanism property key
     * @param defaultValue the default value, if no mapping exists
     * @param <T> the value type
     * @return the mechanism property value
     */
    @SuppressWarnings("unchecked")
    public <T> T getMechanismProperty(String key, T defaultValue) {
        T value = (T) mechanismProperties.get(key);
        return (value == null) ? defaultValue : value;
    }

    @Override
    public boolean equals(final Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        final SequoiadbCredential that = (SequoiadbCredential) o;

        if (!mechanism.equals(that.mechanism)) return false;
        if (!Arrays.equals(password, that.password)) return false;
        if (!source.equals(that.source)) return false;
        if (!userName.equals(that.userName)) return false;
        if (!mechanismProperties.equals(that.mechanismProperties)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = mechanism.hashCode();
        result = 31 * result + userName.hashCode();
        result = 31 * result + source.hashCode();
        result = 31 * result + (password != null ? Arrays.hashCode(password) : 0);
        result = 31 * result + mechanismProperties.hashCode();
        return result;
    }

    @Override
    public String toString() {
        return "SequoiadbCredential{" +
                "mechanism='" + mechanism + '\'' +
                ", userName='" + userName + '\'' +
                ", source='" + source + '\'' +
                ", password=<hidden>"  +
                ", mechanismProperties=" + mechanismProperties +
                '}';
    }
}
