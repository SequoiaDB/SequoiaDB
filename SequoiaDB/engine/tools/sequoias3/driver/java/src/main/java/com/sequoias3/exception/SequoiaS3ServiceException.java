package com.sequoias3.exception;

public class SequoiaS3ServiceException extends SequoiaS3ClientException{

    /** The HTTP status code that was returned with this error */
    private int statusCode;

    /**
     * The error code represented by this exception (ex:
     * InvalidParameterValue).
     */
    private String errorCode;
    /**
     * The error message as returned by the service.
     */
    private String errorMessage;

    public SequoiaS3ServiceException(String errorMessage) {
        super((String)null);
        this.errorMessage = errorMessage;
    }

    /**
     * Sets the HTTP status code that was returned with this service exception.
     *
     * @param statusCode
     *            The HTTP status code that was returned with this service
     *            exception.
     */
    public void setStatusCode(int statusCode) {
        this.statusCode = statusCode;
    }

    /**
     * Returns the HTTP status code that was returned with this service
     * exception.
     *
     * @return The HTTP status code that was returned with this service
     *         exception.
     */
    public int getStatusCode() {
        return statusCode;
    }

    /**
     * @return the human-readable error message provided by the service
     */
    public String getErrorMessage() {
        return errorMessage;
    }

    /**
     * Sets the human-readable error message provided by the service.
     */
    public void setErrorMessage(String value) {
        errorMessage = value;
    }

    /**
     * Sets the code represented by this exception.
     *
     * @param errorCode
     *            The error code represented by this exception.
     */
    public void setErrorCode(String errorCode) {
        this.errorCode = errorCode;
    }

    /**
     * Returns the error code represented by this exception.
     *
     * @return The error code represented by this exception.
     */
    public String getErrorCode() {
        return errorCode;
    }
}
