bigFileNum=3
bigFileSizeMin=100
bigFileSizeMax=400
bigFileSizeUnit=1047576 #M

norFileNum=100
norFileSizeMin=100
norFileSizeMax=400
norFileSizeUnit=1024 #K

function createFile() {
   fileName=$1
   sizeMin=$2
   sizeMax=$3
   unit=$4
   # ${RANDOM}: [0, 32767)
   ((size=${sizeMin}+${RANDOM}%(${sizeMax}-${sizeMin})))
   dd if=/dev/urandom of=${fileName} count=${size} bs=${unit}
}

function main() {
   rm -f ./lob-download/*
   rm -f ./lob-upload-big/*
   rm -f ./lob-upload-nor/*

   cd ./lob-upload-big/
   for((i=0;i<${bigFileNum};i++)); do
      createFile ${i} ${bigFileSizeMin} ${bigFileSizeMax} ${bigFileSizeUnit}
   done
   cd ..


   cd ./lob-upload-nor/
   for((i=0;i<${norFileNum};i++)); do
      createFile ${i} ${norFileSizeMin} ${norFileSizeMax} ${norFileSizeUnit}
   done
   cd ..
}

main
