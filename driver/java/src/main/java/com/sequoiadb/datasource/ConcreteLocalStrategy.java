package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.*;

class ConcreteLocalStrategy extends AbstractStrategy {
    private Random _rand = new Random(47);
    private List<String> _localCoordAddrs = new ArrayList<String>();
    private Set<String> _localNetCardIPs = new HashSet<String>();

    @Override
    public void init(Set<String> addresses, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs) {
        super.init(addresses, _idleConnPairs, _usedConnPairs);
        _localNetCardIPs = getNetCardIPs();
        Set<String> addrsList2Set = new HashSet<String>();
        addrsList2Set.addAll(_addrs);
        _localCoordAddrs.addAll(getLocalCoordIPs(addrsList2Set, _localNetCardIPs));
    }

    @Override
    public String getAddress() {
        String addr = null;
        _lockForAddr.lock();
        try {
            if (_localCoordAddrs.size() >= 1) {
                addr = _localCoordAddrs.get(_rand.nextInt(_localCoordAddrs.size()));
            } else {
                if (_addrs.size() >= 1) {
                    addr = _addrs.get(_rand.nextInt(_addrs.size()));
                }
            }
        } finally {
            _lockForAddr.unlock();
        }
        return addr;
    }

    public void addAddress(String addr) {
        super.addAddress(addr);
        _lockForAddr.lock();
        try {
            if (isLocalAddress(addr, _localNetCardIPs)) {
                _localCoordAddrs.add(addr);
            }
        } finally {
            _lockForAddr.unlock();
        }
    }

    public List<ConnItem> removeAddress(String addr) {
        List<ConnItem> list = super.removeAddress(addr);
        _lockForAddr.lock();
        try {
            if (isLocalAddress(addr, _localNetCardIPs)) {
                _localCoordAddrs.remove(addr);
            }
        } finally {
            _lockForAddr.unlock();
        }
        return list;
    }

    static Set<String> getNetCardIPs() {
        Set<String> localNetCardIPs = new HashSet<String>();
        localNetCardIPs.add("127.0.0.1");
        try {
            Enumeration<NetworkInterface> netcards = NetworkInterface.getNetworkInterfaces();
            if (null == netcards) {
                return localNetCardIPs;
            }
            for (NetworkInterface netcard : Collections.list(netcards)) {
                if (null != netcard.getHardwareAddress()) {
                    List<InterfaceAddress> list = netcard.getInterfaceAddresses();
                    for (InterfaceAddress interfaceAddress : list) {
                        String addr = interfaceAddress.getAddress().toString();
                        if (addr.indexOf("/") >= 0) {// TODO: check in linux
                            String ip = addr.split("/")[1];
                            if (!localNetCardIPs.contains(ip)) {
                                localNetCardIPs.add(ip);
                            }
                        }
                    }
                }
            }
        } catch (SocketException e) {
            throw new BaseException(SDBError.SDB_SYS, "failed to get local ip address");
        }
        return localNetCardIPs;
    }

    static Set<String> getLocalCoordIPs(Set<String> urls, Set<String> localNetCardIPs) {
        Set<String> localCoordAddrs = new HashSet<String>();
        if (localNetCardIPs.size() > 0) {
            for (String url : urls) {
                String ip = url.split(":")[0].trim();
                if (localNetCardIPs.contains(ip))
                    localCoordAddrs.add(url);
            }
        }
        return localCoordAddrs;
    }

    /**
     * @return true or false
     * @fn boolean isLocalAddress(String url)
     * @bref Judge a coord address is in local or not
     */
    static boolean isLocalAddress(String url, Set<String> localIPs) {
        return localIPs.contains(url.split(":")[0].trim());
    }


}
