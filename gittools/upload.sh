#/bin/bash
#BASERELEASE="trunk"
BASERELEASE="trunk"
USERNAME=`whoami`
EMAIL="$USERNAME@outlook.com"
if [[ $# == 1 ]];then
   if [[ $1 == "init" ]];then
      #initialize local config

      git config --global user.name $USERNAME

      git config --global user.email $EMAIL

      #initialize local repository
      rm -rf ~/trunk
     # mkdir ~/trunk
      svn checkout http://192.168.20.11/sequoiadb/trunk
      rm -rf ~/github
      mkdir ~/github
      cd ~/github
      git clone git@github.com:$USERNAME/SequoiaDB.git
      exit
   fi
fi
# svn code checkout
cd ~/$BASERELEASE
if [[ $# == 1 ]];then
   svn update -r $1
fi
echo "checkout"
cd ~/github/SequoiaDB
git checkout master
echo "Current branch is..."
git branch

echo "ready to clone, you have 10 seconds to cancel"
sleep 10

cd ~/$BASERELEASE
rm -rf gittools
cp -r /home/gittools .
gittools/clone.sh ~/github/SequoiaDB


cd ~/$BASERELEASE
svn_version=`svn info | grep Revision`
echo "Current version is $svn_version"
cd ~/github/SequoiaDB
git pull origin master
echo "Wait for adding changes into git"
sleep 10

echo "Add removed/modified/added files into git"
git add -A
git status
echo "Wait before commit into git"
sleep 10

echo "Commit $svn_version into git"
git commit -m "$svn_version"
git push origin master

echo "Done"
