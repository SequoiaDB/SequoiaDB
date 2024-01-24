package com.sequoiadb.lob;

import com.sequoiadb.metaopr.commons.MyUtil;
import org.bson.types.ObjectId;

import java.util.Arrays;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-8-7
 * @Version 1.00
 */
public class LobBean {
    private ObjectId id;
    private byte[] content;
    private byte[] contentMd5;
    private boolean isInSdb = false;

    public LobBean() {
    }

    public LobBean( byte[] content ) {
        this.content = content;
        contentMd5 = MyUtil.getMd5( content );
    }

    public byte[] getContent() {
        return content;
    }

    public void setId( ObjectId id ) {
        this.id = id;
    }

    public ObjectId getId() {
        return id;
    }

    public byte[] getContentMd5() {
        return contentMd5;
    }

    public boolean isInSdb() {
        return isInSdb;
    }

    /**
     * 该lob是否已经保存到sdb
     * 
     * @param inSdb
     */
    public void setInSdb( boolean inSdb ) {
        isInSdb = inSdb;
    }

    @Override
    public String toString() {
        return "LobBean{" + "id=" + id + ", content="
                + Arrays.toString( content ) + ", contentMd5="
                + Arrays.toString( contentMd5 ) + ", isInSdb=" + isInSdb + '}';
    }
}
