package com.sequoias3.dao;

import com.sequoias3.core.Range;
import com.sequoias3.exception.S3ServerException;

import javax.servlet.ServletOutputStream;

public interface DataLob {
    public long getSize();
    public void read(ServletOutputStream outputStream, Range range) throws S3ServerException;
}
