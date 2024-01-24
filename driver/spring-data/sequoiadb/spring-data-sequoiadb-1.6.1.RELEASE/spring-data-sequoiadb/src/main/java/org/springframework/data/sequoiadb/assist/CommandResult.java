package org.springframework.data.sequoiadb.assist;

import com.sequoiadb.exception.BaseException;
import org.bson.BasicBSONObject;

/**
 * Created by tanzhaobo on 2017/9/1.
 */
public class CommandResult extends BasicBSONObject {
    // TODO: this is a useless class, maybe we don't need to export it, so, try to remove it

    private  ServerAddress _address;
    private int _errCode;
    private String _errMsg;
    private String _detail;

    CommandResult(ServerAddress address) {
        this._errCode = 0;
        this._errMsg = "";
        this._detail = "";
        this._address = address;
    }

    CommandResult(int errCode, String errMsg, String detail, ServerAddress address) {
        this._errCode = errCode;
        this._errMsg = errMsg;
        this._detail = detail;
        this._address = address;
    }

    /**
     * gets the "ok" field which is the result of the command
     * @return True if ok
     */
    public boolean ok(){
        return _errCode == 0 ? true : false;
    }

    /**
     * gets the "errmsg" field which holds the error message
     * @return The error message or null
     */
    public String getErrorMessage(){
        return String.format("%s(%d), %s",_errMsg, _errCode, _detail);
    }

    /**
     * utility method to create an exception with the command name
     * @return The sdb exception or null
     */
    public BaseException getException() {
        return new BaseException(_errCode, String.format("%s, %s", _errMsg, _detail));
    }

    /**
     * throws an exception containing the cmd name, in case the command failed, or the "err/code" information
     * @throws BaseException
     */
    public void throwOnError() {
        if (!ok()) {
            throw getException();
        }
    }

    public ServerAddress getServerUsed() {
        return _address;
    }

}
