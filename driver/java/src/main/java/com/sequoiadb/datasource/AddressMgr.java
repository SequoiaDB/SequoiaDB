/*
 * Copyright 2023 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.log.Log;
import com.sequoiadb.log.LogFactory;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import java.net.*;
import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

class AddressMgr implements IAddressMgr {
    private final static String LOCATION_SEPARATOR = ".";
    private final static Log log = LogFactory.getLog(AddressMgr.class);

    private final LinkedHashMap<String, ServerAddress> addressMap;
    private final String location;
    private final LocationPriorityCounter counter;
    private final List<String> localIpList;
    private final ReentrantReadWriteLock rwLock;

    AddressMgr(List<String> addressList, String location) {
        this.addressMap = new LinkedHashMap<>();
        this.location = location;
        this.counter = new LocationPriorityCounter();
        this.localIpList = getNetCardIPs();
        this.rwLock = new ReentrantReadWriteLock();

        for (String address : addressList) {
            if (address == null || address.isEmpty()) {
                continue;
            }

            String addr = Helper.parseAddress(address);
            ServerAddress serAddr = new ServerAddress(addr);
            serAddr.setLocal(isLocalAddress(addr));
            this.addressMap.put(addr, serAddr);
        }
        if (this.addressMap.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "No available address: " + addressList);
        }
    }

    public List<String> updateAddressInfo(BSONObject addrInfoObj, UpdateType type) {
        Map<String, String> addrInfoMap = parseAddressInfo(addrInfoObj);

        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            List<String> decList = null;
            switch (type) {
                case ADDRESS:
                    decList = updateAddressInfo(addrInfoMap);
                    break;
                case LOCATION:
                    updateLocationInfo(addrInfoMap);
                    break;
                default:
                    break;
            }
            return decList;
        } finally {
            lock.unlock();
        }
    }

    public List<ServerAddress> getAddress() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            if (counter.getHighNum() > 0) {
                return getAddressByPriority(LocationPriority.HIGH);
            } else if (counter.getMediumNum() > 0) {
                return getAddressByPriority(LocationPriority.MEDIUM);
            } else {
                return getAddressByPriority(LocationPriority.LOW);
            }
        } finally {
            lock.unlock();
        }
    }

    private List<ServerAddress> getAddressByPriority(LocationPriority priority) {
        List<ServerAddress> serAddrLst = new ArrayList<>();
        for (ServerAddress serAddr : addressMap.values()) {
            if (!serAddr.isEnable()) {
                continue;
            }
            if (serAddr.getLocationPriority() == priority) {
                serAddrLst.add(serAddr);
            }
        }
        return serAddrLst;
    }

    public List<ServerAddress> getNormalAddress() {
        return getAddressByStatus(true);
    }

    public List<ServerAddress> getAbnormalAddress() {
        return getAddressByStatus(false);
    }

    private List<ServerAddress> getAddressByStatus(boolean isEnable) {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            List<ServerAddress> serAddrLst = new ArrayList<>();
            for (ServerAddress serAddr : addressMap.values()) {
                if (serAddr.isEnable() == isEnable) {
                    serAddrLst.add(serAddr);
                }
            }
            return serAddrLst;
        } finally {
            lock.unlock();
        }
    }

    public List<ServerAddress> getLocalAddress() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            List<ServerAddress> addrLst = new ArrayList<>();
            for (ServerAddress addr : addressMap.values()) {
                if (addr.isLocal()) {
                    addrLst.add(addr);
                }
            }
            return addrLst;
        } finally {
            lock.unlock();
        }
    }

    public int getNormalAddressSize() {
        return getNormalAddress().size();
    }

    public int getAbnormalAddressSize() {
        return getAbnormalAddress().size();
    }

    public int getLocalAddressSize() {
        return getLocalAddress().size();
    }

    public boolean checkAddress(String address) {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            ServerAddress serAddr = addressMap.get(address);
            if (serAddr == null || !serAddr.isEnable()) {
                return false;
            }
            switch (serAddr.getLocationPriority()) {
                case HIGH:
                    return counter.getHighNum() > 0;
                case MEDIUM:
                    return (counter.getHighNum() == 0) && (counter.getMediumNum() > 0);
                case LOW:
                    return (counter.getHighNum() == 0) && (counter.getMediumNum() == 0);
                default:
                    return false;
            }
        } finally {
            lock.unlock();
        }
    }

    public void addAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            if (addressMap.containsKey(address)) {
                log.info(String.format("Already exist address: %s", address));
            } else {
                ServerAddress serAddr = new ServerAddress(address);
                serAddr.setLocal(isLocalAddress(address));
                // SyncAddressInfoTask will update location for the address later

                addressMap.put(address, serAddr);
                counter.inc(serAddr.getLocationPriority());
                log.info(String.format("Add address: %s", address));
            }
        } finally {
            lock.unlock();
        }
    }

    public void removeAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            ServerAddress serAddr = addressMap.remove(address);
            if (serAddr != null && serAddr.isEnable()) {
                counter.dec(serAddr.getLocationPriority());
            }
        } finally {
            lock.unlock();
        }
    }

    public void enableAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            ServerAddress serAddr = addressMap.get(address);
            if (serAddr != null && !serAddr.isEnable()) {
                serAddr.setEnable(true);
                counter.inc(serAddr.getLocationPriority());
            }
        } finally {
            lock.unlock();
        }
    }

    public void disableAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            ServerAddress serAddr = addressMap.get(address);
            if (serAddr != null && serAddr.isEnable()) {
                serAddr.setEnable(false);
                counter.dec(serAddr.getLocationPriority());
            }
        } finally {
            lock.unlock();
        }
    }

    public String getLocation() {
        return this.location;
    }

    public String getAddressSnapshot() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            int localNum = 0;
            int normalNum = 0;
            int abnormalNum = 0;
            for (ServerAddress addr : addressMap.values()) {
                if (addr.isLocal()) {
                    localNum++;
                }
                if (addr.isEnable()) {
                    normalNum++;
                } else {
                    abnormalNum++;
                }
            }
            return String.format("location: %s, normal address: %d, abnormal address: %d, local address: %d",
                    location, normalNum, abnormalNum, localNum);
        } finally {
            lock.unlock();
        }
    }

    // map<ip address, location name>
    private List<String> updateAddressInfo(Map<String, String> addrInfo) {
        List<String> incList = new ArrayList<>();
        List<String> decList = new ArrayList<>();

        for (String addr : addressMap.keySet()) {
            if (!addrInfo.containsKey(addr)) {
                decList.add(addr);
            }
        }

        for (String addr : addrInfo.keySet()) {
            if (!addressMap.containsKey(addr)) {
                incList.add(addr);
            }
        }

        for (String addr : decList) {
            ServerAddress serAddr = addressMap.remove(addr);
            if (serAddr.isEnable()) {
                counter.dec(serAddr.getLocationPriority());
            }
        }

        for (String addr : incList) {
            ServerAddress serAddr = new ServerAddress(addr);
            serAddr.setLocal(isLocalAddress(addr));
            serAddr.setLocation(addrInfo.get(addr));
            serAddr.setLocationPriority(calAffinity(location, serAddr.getLocation()));

            addressMap.put(addr, serAddr);
            counter.inc(serAddr.getLocationPriority());
        }
        if (incList.size() > 0 || decList.size() > 0) {
            log.info(String.format("update address success, increase address: %s, decrease address: %s",
                    incList, decList));
        }
        return decList;
    }

    // map<ip address, location name>
    private void updateLocationInfo(Map<String, String> addrInfo) {
        for (String addr : addrInfo.keySet()) {
            ServerAddress serAddr = addressMap.get(addr);
            if (serAddr == null) {
                continue;
            }

            String location = addrInfo.get(addr);
            if (location.equals(serAddr.getLocation())) {
                continue;
            }

            serAddr.setLocation(location);
            serAddr.setLocationPriority(calAffinity(this.location, location));
        }

        counter.reset();
        for (ServerAddress serAddr : addressMap.values()) {
            if (serAddr.isEnable()) {
                counter.inc(serAddr.getLocationPriority());
            }
        }

        int lowNum = addressMap.size() - counter.getHighNum() - counter.getMediumNum();
        log.info("Successfully updated location information for addresses, the number of addresses in different " +
                "levels: HIGH = " + counter.getHighNum() + ", MEDIUM = " + counter.getMediumNum() + " ,LOW = " + lowNum);
    }

    private boolean isLocalAddress(String address) {
        return localIpList.contains(address.split(Helper.ADDRESS_SEPARATOR)[0]);
    }

    static LocationPriority calAffinity(String location1, String location2) {
        if (location1 == null || location1.equals("")) {
            return LocationPriority.LOW;
        }
        if (location2 == null || location2.equals("")) {
            return LocationPriority.LOW;
        }

        // e.g. guangzhou.nansha and guangzhou.nansha
        if (location1.equals(location2)) {
            return LocationPriority.HIGH;
        }

        // e.g. guangzhou.nansha and guangzhou, GUANGZHOU.panyu
        int post1 = location1.indexOf(LOCATION_SEPARATOR);
        if (post1 == -1) {
            post1 = location1.length();
        }
        int post2 = location2.indexOf(LOCATION_SEPARATOR);
        if (post2 == -1) {
            post2 = location2.length();
        }

        String pref1 = location1.substring(0, post1);
        String pref2 = location2.substring(0, post2);
        if (pref1.equalsIgnoreCase(pref2)) {
            return LocationPriority.MEDIUM;
        } else {
            return LocationPriority.LOW;
        }
    }

    // return map<address, location>
    private Map<String, String> parseAddressInfo(BSONObject obj) {
        Map<String, String> addrInfo = new HashMap<>();
        String addr = "";
        String location;
        BaseException exp = new BaseException(SDBError.SDB_SYS, "Invalid address information got from catalog");

        if (obj == null) throw exp;

        BasicBSONList arr = (BasicBSONList) obj.get("Group");
        if (arr == null) throw exp;

        Object[] objArr = arr.toArray();
        for (Object o : objArr) {
            BSONObject subObj = (BasicBSONObject) o;

            String hostName = (String) subObj.get("HostName");
            if (hostName == null || hostName.trim().isEmpty()) throw exp;

            location = (String) subObj.get("Location");
            // coord node maybe not set location
            if (location == null) {
                location = "";
            }

            String svcName;
            BasicBSONList subArr = (BasicBSONList) subObj.get("Service");
            if (subArr == null) throw exp;

            Object[] subObjArr = subArr.toArray();
            for (Object value : subObjArr) {
                BSONObject subSubObj = (BSONObject) value;

                Integer type = (Integer) subSubObj.get("Type");
                if (type == null) throw exp;
                if (type != 0) {
                    continue;
                }

                svcName = (String) subSubObj.get("Name");
                if (svcName == null || svcName.trim().isEmpty()) throw exp;

                try {
                    addr = Helper.parseHostName(hostName.trim()) + ":" + svcName.trim();
                    addrInfo.put(addr, location);
                } catch (Exception e) {
                    // ignore
                }
                break;
            }
        }
        return addrInfo;
    }

    private static List<String> getNetCardIPs() {
        List<String> localIPs = new ArrayList<>();
        localIPs.add("127.0.0.1");
        try {
            Enumeration<NetworkInterface> netCards = NetworkInterface.getNetworkInterfaces();
            if (netCards == null) {
                return localIPs;
            }
            for (NetworkInterface netCard : Collections.list(netCards)) {
                if (netCard.getHardwareAddress() == null) {
                    continue;
                }
                List<InterfaceAddress> list = netCard.getInterfaceAddresses();
                for (InterfaceAddress interfaceAddress : list) {
                    String addr = interfaceAddress.getAddress().getHostAddress();
                    localIPs.add(addr);
                }
            }
        } catch (SocketException e) {
            throw new BaseException(SDBError.SDB_SYS, "Failed to get local ip address");
        }
        return localIPs;
    }
}

enum LocationPriority {
    HIGH,
    MEDIUM,
    LOW
}

class LocationPriorityCounter {
    private int highNum;
    private int mediumNum;

    LocationPriorityCounter() {
        reset();
    }

    void incHighNum() {
        this.highNum += 1;
    }

    void decHigNum() {
        this.highNum -= 1;
    }

    void incMediumNum() {
        this.mediumNum += 1;
    }

    void decMediumNum() {
        this.mediumNum -= 1;
    }

    int getHighNum() {
        return highNum;
    }

    int getMediumNum() {
        return mediumNum;
    }

    void reset() {
        this.highNum = 0;
        this.mediumNum = 0;
    }

    void inc(LocationPriority priority) {
        switch (priority) {
            case HIGH:
                incHighNum();
                break;
            case MEDIUM:
                incMediumNum();
                break;
            default:
                // do nothing
        }
    }

    void dec(LocationPriority priority) {
        switch (priority) {
            case HIGH:
                decHigNum();
                break;
            case MEDIUM:
                decMediumNum();
                break;
            default:
                // do nothing
        }
    }
}
