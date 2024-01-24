package com.sequoiadb.builddata;

public class BuildException extends Exception {

    private static final long serialVersionUID = -218223203783908559L;

    public BuildException() {
        super();
    }

    public BuildException(String message) {
        super(message);
    }

    public BuildException(Throwable e) {
        super(e);
    }

    public BuildException(String message, Throwable e) {
        super(message, e);
    }
}
