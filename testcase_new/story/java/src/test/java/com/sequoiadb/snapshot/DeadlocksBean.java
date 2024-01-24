package com.sequoiadb.snapshot;

import org.bson.BSONObject;

/**
 * @Description: Deadlocks class
 * @Author Yang Qincheng
 * @Date 2021.09.02
 */
public class DeadlocksBean {
    private int deadlockID = -1;
    private String transID = null;
    private int degree = -1;
    private long cost = -1;
    private String relatedID = null;
    private long sessionID = -1;
    private int groupID = -1;
    private int nodeID = -1;

    DeadlocksBean( BSONObject obj ){
        this.deadlockID = (int)obj.get("DeadlockID");
        this.transID = (String) obj.get("TransactionID");
        this.degree = (int)obj.get("Degree");
        this.cost = (long)obj.get("Cost");
        this.relatedID = (String)obj.get("RelatedID");
        this.sessionID = (long)obj.get("SessionID");
        this.groupID = (int)obj.get("GroupID");
        this.nodeID = (int)obj.get("NodeID");
    }
    public boolean check(){
        if ( deadlockID == -1 || transID == null || degree == -1 || cost == -1 || relatedID == null ||
                sessionID == -1 || groupID == -1 || nodeID == -1 ){
            return false;
        }else {
            return true;
        }
    }

    public int getDeadlockID() {
        return deadlockID;
    }

    public long getSession(){
        return this.sessionID;
    }

    public String getTransID(){
        return this.transID;
    }

    public boolean compareTo(DeadlocksBean o) {
        if ( this.degree > o.degree){
            return true;
        }else if( this.cost <= o.cost ){
            return true;
        }else {
            return false;
        }
    }
}
