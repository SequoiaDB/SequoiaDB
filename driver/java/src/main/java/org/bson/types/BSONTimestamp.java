// BSONTimestamp.java

/**
 *      Copyright (C) 2008 10gen Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

package org.bson.types;

import java.io.Serializable;
import java.sql.Timestamp;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * this is used for storing timestamp.
 * for storing normal dates in database, you should use java.util.Date
 * <b>time</b> is seconds since epoch
 * <b>inc<b> is an ordinal
 */
public class BSONTimestamp implements Serializable {

    private static final long serialVersionUID = -3268482672267936464L;
    
    static final boolean D = Boolean.getBoolean( "DEBUG.DBTIMESTAMP" );

    /**
     * Construct an empty BSONTimestamp.
     */
    public BSONTimestamp(){
        _inc = 0;
        _time = new Date(0L );
    }

    /**
     * Construct BSONTimestamp.
     * @param time seconds since epoch.
     * @param inc microseconds in range of [0us, 999999us], while the 'inc' is out of range,
     *             the carry will occur.
     */
    public BSONTimestamp(int time, int inc ) {
        if (inc < 0 || inc >= 1000000) {
            time += inc / 1000000;
            inc = inc % 1000000;
            if (inc < 0) {
                time -= 1;
                inc += 1000000;
            }
        }
        _time = new Date(time * 1000L);
        _inc = inc;
    }

    /**
     * Construct BSONTimestamp by java.util.Date.
     */
    public BSONTimestamp(Date date) {
        this((int)(date.getTime() / 1000), (int)(date.getTime() % 1000) * 1000);
    }

    /**
     * Construct BSONTimestamp by java.sql.Timestamp.
     * The precision of BSONTimestamp is microsecond and Timestamp is nanosecond,
     * so there may have a loss of nanoseconds.
     */
    public BSONTimestamp(Timestamp timestamp) {
        this((int)(timestamp.getTime() / 1000),
            timestamp.getNanos() / 1000);
    }

    /**
     * @return Date of time and inc in milliseconds since epoch.
     */
    public Date toDate() {
        return new Date(getTime() * 1000L + getInc() / 1000L);
    }

    /**
     * @return Timestamp of time in milliseconds and inc in nanoseconds since epoch
     */
    public Timestamp toTimestamp() {
        Timestamp ts = new Timestamp(getTime() * 1000L);
        ts.setNanos(getInc() * 1000);
        return ts;
    }

    /**
     * @return get time in seconds since epoch
     */
    public int getTime(){
        if ( _time == null )
            return 0;
        return (int)(_time.getTime() / 1000);
    }

    /**
     * @return get time in microseconds since epoch
     */
    public int getInc(){
        return _inc;
    }

    public String toString(){
    	DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-HH.mm.ss");
    	String strDate = formatter.format(_time);

        return "{ $timestamp : " + strDate + "." + _inc + " }";
    }

    /**
     * @return get time in seconds since epoch
     */
    public Date getDate() {
    	if(_time == null)
    		return null;
    	return _time;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this)
            return true;
        if (obj instanceof BSONTimestamp) {
            BSONTimestamp t2 = (BSONTimestamp) obj;
            return getTime() == t2.getTime() && getInc() == t2.getInc();
        }
        return false;
    }

    final int _inc;
    final Date _time;
}
