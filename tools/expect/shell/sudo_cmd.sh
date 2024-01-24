#!/bin/sh

if { $argc < 3 } {
   puts "invalid argument"
   puts "usage: expect sudo_cmd.sh \[timeout(seconds)] \[password] \[command] \[arg1 arg2 ...]"
   exit 2
}

set times [lindex $argv 0]
set pwd   [lindex $argv 1]
set args  [lrange $argv 2 end]

set timeout $times

spawn sudo su -l -c "$args"

expect {
   "*sudo] password for *" {
      send "$pwd\r"
   }
   "root's password*" {
      send "$pwd\r"
   }
   "*assword*" {
      send "$pwd\r"
   }
   timeout {
      foreach {pid spawnid os_error_flag value} [wait] break
      exit $value
   }
   eof {
      foreach {pid spawnid os_error_flag value} [wait] break
      exit $value
   }
}

expect {
   "*sudo] password for *" {
      send \x03
      exit 2
   }
   "root's password*" {
      send \x03
      exit 2
   }
   "*assword*" {
      send \x03
      exit 2
   }
   timeout {
      foreach {pid spawnid os_error_flag value} [wait] break
   }
   eof {
      foreach {pid spawnid os_error_flag value} [wait] break
   }
}

exit $value

