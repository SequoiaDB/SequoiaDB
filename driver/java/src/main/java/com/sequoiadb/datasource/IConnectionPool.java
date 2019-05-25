package com.sequoiadb.datasource;

import com.sequoiadb.base.Sequoiadb;

import java.util.Iterator;
import java.util.List;


class Pair {
    private ConnItem _item;
    private Sequoiadb _sdb;

    public Pair(ConnItem item, Sequoiadb sdb) {
        _item = item;
        _sdb = sdb;
    }

    public ConnItem first() {
        return _item;
    }

    public Sequoiadb second() {
        return _sdb;
    }
}


interface IConnectionPool {
    public Sequoiadb poll(ConnItem pos);

    public ConnItem poll(Sequoiadb sdb);

    public void insert(ConnItem item, Sequoiadb sdb);

    public Iterator<Pair> getIterator();

    public int count();

    public boolean contains(Sequoiadb sdb);

    public List<ConnItem> clear();
}
