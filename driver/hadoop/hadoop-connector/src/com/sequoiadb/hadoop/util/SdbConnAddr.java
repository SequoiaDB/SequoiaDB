package com.sequoiadb.hadoop.util;

public class SdbConnAddr {
	private String host;
	private int port;
	public String getHost() {
		return host;
	}
	public void setHost(String host) {
		this.host = host;
	}
	public int getPort() {
		return port;
	}
	public void setPort(int port) {
		this.port = port;
	}
	
	public SdbConnAddr(String url) {
		if(url==null){
			throw new IllegalArgumentException("the arguements is null");			
		}
		
		String[] splitList=url.split(":");
		if(splitList.length!=2){
			throw new IllegalArgumentException("the arguements is wrong");	
		}
		
		this.host=splitList[0];
		this.port=Integer.parseInt(splitList[1]);
		
	}
	
	public SdbConnAddr() {
		super();
	}
	public SdbConnAddr(String host, int port) {
		super();
		this.host = host;
		this.port = port;
	}
	@Override
	public int hashCode() {
		return host.hashCode()*31+port;
	}
	@Override
	public boolean equals(Object obj) {
		if(this==obj){
			return true;
		}
		
		if(!(obj instanceof SdbConnAddr)){
			return false;
		}
		
		SdbConnAddr  other=(SdbConnAddr)obj;
		
		if(this.getHost().equals(other.getHost())&&this.getPort()==other.getPort()){
			return true;
		}else{
			return false;
		}
		
	}
	@Override
	public String toString() {
		return String.format("%s:%d",host,port);
	}
	
	
	
	
}
