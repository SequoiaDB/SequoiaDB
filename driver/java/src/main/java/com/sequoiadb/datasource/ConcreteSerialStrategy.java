package com.sequoiadb.datasource;

class ConcreteSerialStrategy extends AbstractStrategy {

    private int _counter = 0;

    @Override
    public String getAddress() {
        String addr = null;
        _lockForAddr.lock();
        try {
            if (_addrs.size() >= 1) {
                addr = _addrs.get((0x7fff & (_counter++)) % (_addrs.size()));
            }
        } finally {
            _lockForAddr.unlock();
        }
        return addr;
    }


}
