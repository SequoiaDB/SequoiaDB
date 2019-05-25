package com.sequoiadb.datasource;

import java.util.List;
import java.util.Set;


enum Operation {
    GET,
    DELETE
}

enum ItemStatus {
    IDLE,
    USED
}

interface IConnectStrategy {
    public void init(Set<String> addresses, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs);

    public ConnItem pollConnItem(Operation opr);

    public String getAddress();

    /*
     ItemStatus  incDecItemCount  meaning
       IDLE      >0    one connection had been add to idle pool,
                       strategy need to record the info of that idle connection.
       IDLE      <0    one connection had been removed from idle pool,
                       strategy need to remove the info of that idle connection
       USED      >0    one connection was filled to used pool,
                       strategy need to increase amount of used connection with specified address
       USED      <0    one connection was got out from the used pool,
                       strategy need to decrease amount of used connection with specified address
     */
    public void update(ItemStatus itemStatus, ConnItem connItem, int incDecItemCount);

    public void addAddress(String addr);

    public List<ConnItem> removeAddress(String addr);
}
