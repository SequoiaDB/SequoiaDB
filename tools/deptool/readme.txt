# all the variable in this file
common.sh
 
# all the function in this file
deploy_function.sh 

# remote machine will execute command
# include clear the env , install the new sequoiadb software
# and start the sdbcm , sequoiadb engine
remote_deploy.sh

# local machine or master machine will execute command
# include send remote_deploy.sh , software or some files to remote machine
# and let the remote machine execute the command
# and deploy the sequoiadb env , catalog and data nodes
sequoiadb_deploy.sh
