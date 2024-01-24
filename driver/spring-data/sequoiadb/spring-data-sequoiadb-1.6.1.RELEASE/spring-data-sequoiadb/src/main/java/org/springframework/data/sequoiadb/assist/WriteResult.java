package org.springframework.data.sequoiadb.assist;

// WriteResult.java


import com.sequoiadb.exception.SDBError;

/**
 * This class lets you access the results of the previous write.
 * if you have STRICT mode on, this just stores the result of that getLastError call
 * if you don't, then this will actually do the getlasterror call.
 * if another operation has been done on this connection in the interim, calls will fail
 */
@SuppressWarnings("deprecation")
public class WriteResult {

    private int _errCode;
    private String _errType;
    private String _detail;
    private ServerAddress _address;

    // TODO: remove it
//    WriteResult () {}

    WriteResult (ServerAddress address) {
        this._errCode = 0;
        this._errType = "";
        this._detail = "";
        this._address = address;
    }

    WriteResult(int errCode, String detail, ServerAddress address){
        this._errCode = errCode;
        this._errType = SDBError.getSDBError(errCode).getErrorType();
        this._detail = detail;
        this._address = address;
    }

    WriteResult(SDBError error, String detail, ServerAddress address){
        this._errCode = error.getErrorCode();
        this._errType = error.getErrorType();
        this._detail = detail;
        this._address = address;
    }

    void setExpInfo(com.sequoiadb.exception.BaseException e, String ... details) {
        this._errCode = e.getErrorCode();
        this._errType = e.getErrorType();
        this._detail = e.getMessage();
        for (int i = 0; i < details.length; i++) {
            this._detail += ", " + details[i];
        }
    }

    /**
     * Gets the last result from getLastError().
     *
     * @return the result of the write operation
     * @deprecated Use the appropriate {@code WriteConcern} and rely on the write operation to throw an exception on failure.  For
     * successful writes, use the helper methods to retrieve specific values from the write response.
     * @see #getN()
     * @see #getUpsertedId()
     * @see #isUpdateOfExisting()
     */
    @Deprecated
    public CommandResult getCachedLastError(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Gets the last {@link WriteConcern} used when calling getLastError().
     *
     * @return the write concern that was applied to the write operation
     */
    public WriteConcern getLastConcern(){
        throw new UnsupportedOperationException("not supported!");
    }

    /**
     * Calls {@link WriteResult#getLastError(com.sequoiadb.WriteConcern)} with a null write concern.
     *
     * @return the response to the write operation
     * @throws BaseException
     * @deprecated Use the appropriate {@code WriteConcern} and allow the write operation to throw an exception on failure.  For
     * successful writes, use the helper methods to retrieve specific values from the write response.
     * @see #getN()
     * @see #getUpsertedId()
     * @see #isUpdateOfExisting()
     */
    @Deprecated
    public synchronized CommandResult getLastError(){
        return getLastError(null);
    }

    /**
     * This method does following:
     * - returns the existing CommandResult if concern is null or less strict than the concern it was obtained with
     * - otherwise attempts to obtain a CommandResult by calling getLastError with the concern
     * @param concern the concern
     * @return the response to the write operation
     * @throws BaseException
     * @deprecated Use the appropriate {@code WriteConcern} and rely on the write operation to throw an
     * exception on failure.  For successful writes, use the helper methods to retrieve specific values from the write response.
     * @see #getN()
     * @see #getUpsertedId()
     * @see #isUpdateOfExisting()
     */
    @Deprecated
    public synchronized CommandResult getLastError(WriteConcern concern){
        return new CommandResult(_errCode, _errType, _detail, _address);
    }


    /**
     * Gets the error message from the {@code "err"} field).
     *
     * @return the error
     * @throws BaseException
     * @deprecated There should be no reason to use this method.  The error message will be in the exception thrown for an
     * unsuccessful write operation.
     */
    @Deprecated
    public String getError(){
        Object foo = getField( "err" );
        if ( foo == null )
            return null;
        return foo.toString();
    }

    /**
     * Gets the "n" field, which contains the number of documents
     * affected in the write operation.
     * @return the number of documents modified by the write operation
     * @throws BaseException
     */
    public int getN(){
        return 0;
    }

    /**
     * Gets the _id value of an upserted document that resulted from this write.  Note that for SequoiaDB servers prior to version 2.6,
     * this method will return null unless the _id of the upserted document was of type ObjectId.
     *
     * @return the value of the _id of an upserted document
     * @since 2.12
     */
    public Object getUpsertedId() {
        throw new UnsupportedOperationException("not supported!");
    }


    /**
     * Returns true if this write resulted in an update of an existing document.
     *
     * @return whether the write resulted in an update of an existing document.
     * @since 2.12
     */
    public boolean isUpdateOfExisting() {
        throw new UnsupportedOperationException("not supported!");
    }


    /**
     * Gets a field from the response to the write operation.
     *
     * @param name field name
     * @return the value of the field with the given name
     * @throws BaseException
     * @deprecated There should be no reason to use this method.  To get specific fields from a successful write,
     * use the helper methods provided.  Any error-related fields will be in the exception thrown for an unsuccessful write operation.
     * @see #getN()
     * @see #getUpsertedId()
     * @see #isUpdateOfExisting()
     */
    @Deprecated
    public Object getField( String name ){
        return null;
    }

    /**
     * Returns whether or not the result is lazy, meaning that getLastError was not called automatically
     * @return true if the result is lazy
     * @deprecated Call {@code WriteResult.getLastConcern().isAcknowledged()} instead
     */
    @Deprecated
    public boolean isLazy(){
        return false;
    }

    @Override
    public String toString(){
        return this._errType + "(" + this._errCode + ")" + ": " + this._detail;
    }


}

