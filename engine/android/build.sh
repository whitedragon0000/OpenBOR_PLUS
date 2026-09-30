#!/bin/bash
#
# build the android apk !
# 
#set build version, android home temp variable and build
export ANDROID_SDK_ROOT="$HOME/.android"
export ANDROID_HOME="$ANDROID_SDK_ROOT"
export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/21.4.7075529"
export JAVA_HOME="$ANDROID_HOME/jdk-17.0.1"

# function to try to download files
downloader()
{
	DL_NAME=$1
	DL_URL=$2
	DL_MD5=$3
	DL_PATH=$ANDROID_HOME

	for i in {1..5}
		do
	
			if [ $i == 5 ]; # tried 5 times now give up!
			then 
				echo "error: failed to download $DL_NAME now aboting"
				exit
			fi
	
			if [ ! -f $DL_PATH/$DL_NAME ];
			then
				wget --no-check-certificate $DL_URL -O $DL_PATH/$DL_NAME
			fi
		
			if md5sum $DL_PATH/$DL_NAME | grep -i $DL_MD5 > /dev/null;
			then 
				cd $DL_PATH
				tar -xf $DL_NAME -C "$ANDROID_HOME"
				break
			else 
				rm $DL_PATH/$DL_NAME
			fi
		done
}



# make sdk directory if missing
if [ ! -d $ANDROID_HOME ];
then
	cd ~/
	mkdir $ANDROID_HOME
fi

#get command line tools if missing
if [ ! -d "$ANDROID_HOME/cmdline-tools" ];
then
downloader commandlinetools-linux-13114758_latest.tar.gz "https://app.box.com/index.php?rm=box_download_shared_file&shared_name=8amw5xzga9lfuv8rfad4bit5cgx2w94g&file_id=f_1875969786508" 29f56e9f159d02b33dd3ccd72a3a63ce
fi

#get JDK 17 if missing
if [ ! -d "$JAVA_HOME" ];
then
downloader openjdk-17.0.1_linux-x64_bin.tar.gz "https://app.box.com/index.php?rm=box_download_shared_file&shared_name=5r0urbd5f9i2xvmgd2nnw9xpuvpo4atr&file_id=f_1875964794936" bfb0c5653a3171a7e9430f81f70b3ec1
fi

#sign all licenses
if [ ! -d ~/.android/licenses ];
then
	cd "$ANDROID_HOME"
	bash cmdline-tools/bin/sdkmanager --sdk_root=./ --licenses
fi


cd $(dirname $(readlink -f $0))
cd ../
chmod +x version.sh
./version.sh
cd android
./gradlew clean
./gradlew assembleDebug
