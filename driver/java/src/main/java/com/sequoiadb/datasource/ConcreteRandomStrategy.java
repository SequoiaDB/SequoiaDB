package com.sequoiadb.datasource;

import java.util.Random;

class ConcreteRandomStrategy extends AbstractStrategy {
    private Random _rand = new Random(47);

    @Override
    public String getAddress() {
        String addr = null;
        _lockForAddr.lock();
        try {
            if (_addrs.size() >= 1) {
                addr = _addrs.get(_rand.nextInt(_addrs.size()));
            }
        } finally {
            _lockForAddr.unlock();
        }
        return addr;
    }

}
