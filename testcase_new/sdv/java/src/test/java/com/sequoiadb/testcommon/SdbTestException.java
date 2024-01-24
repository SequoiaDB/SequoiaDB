package com.sequoiadb.testcommon;

public class SdbTestException extends RuntimeException {

    private static final long serialVersionUID = -6115487863398926195L;

    private SdbTestError error;
    private String detail;

    /**
     * @param error
     *            The enumeration object of sdb test error.
     * @param detail
     *            The error detail.
     * @param e
     *            The exception used to build exception chain.
     */
    public SdbTestException( SdbTestError error, String detail, Exception e ) {
        this.error = error;
        this.detail = detail;
        this.initCause( e );
    }

    public SdbTestException( String detail, Exception e ) {
        this( SdbTestError.getTestError( -10000 ), detail, e );
    }

    public SdbTestException( String detail ) {
        this( SdbTestError.getTestError( -10000 ), detail, null );
    }

    /**
     * @brief Get the error message.
     * @return The error message.
     */
    @Override
    public String getMessage() {
        if ( detail != null && detail != "" ) {
            if ( error != null ) {
                return error.toString() + ", detail: " + detail;
            } else {
                return detail;
            }
        } else if ( error != null ) {
            return error.toString();
        } else {
            return "Unknown Error";
        }
    }

    /**
     * @brief Get the error type.
     * @return The error type.
     */
    public String getErrorType() {
        return error != null ? error.getErrorType() : "Unknown Type";
    }

    /**
     * @brief Get the error code.
     * @return The error code.
     */
    public int getErrorCode() {
        return error != null ? error.getErrorCode() : 0;
    }
}
