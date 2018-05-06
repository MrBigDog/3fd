#!/bin/bash

UBUNTU=$(cat /etc/issue | grep -i ubuntu | awk -F" " '{print $2}' | awk -F"." '{print $1}')
AMAZON=$(cat /etc/issue | grep -i "amazon linux ami release" | awk -F" " '{print $5}' | awk -F"." '{print $1}')

export SetColorToRED='\033[0;31m';
export SetColorToLightRED='\033[1;31m';
export SetColorToYELLOW='\033[0;33m';
export SetColorToLightYELLOW='\033[1;33m';
export SetColorToGREEN='\033[0;32m';
export SetColorToLightGREEN='\033[1;32m';
export SetColorToBLUE='\033[0;34m';
export SetColorToLightBLUE='\033[1;34m';
export SetColorToWHITE='\033[0;37m';
export SetNoColor='\033[0m';

# # #
# SETUP DEV ENV
#

if [ -n "$UBUNTU" ] && [ $UBUNTU -gt 14 ]; then
    printf "${SetColorToYELLOW}Checking dependencies...${SetNoColor}\n"
    apt install --assume-yes cmake curl git libc++1 unzip tar

    HAS_GXX7=$(dpkg -l | grep 'g++-[7-9]')

    if [ -z "$HAS_GXX7" ]; then
        while true; do
            read -p "VCPKG requires GNU C++ Compiler v7+. Do you wish to upgrade?" yn
            case $yn in
                [Nn]* ) return;;
                * ) echo "Please answer yes or no.";;
            esac
        done
        printf "${SetColorToYELLOW}Upgrading g++..${SetNoColor}\n"
        add-apt-repository ppa:ubuntu-toolchain-r/test -y
        apt update -y
        apt install g++-7 -y
    fi

    HAS_MSSQL_TOOLS=$(dpkg -l | grep "mssql-tools")

    # MSSQL TOOLS not yet installed?
    if [ -z "$HAS_MSSQL_TOOLS" ]; then
        printf "${SetColorToYELLOW}Installing MSSQL tools...${SetNoColor}\n"
        curl https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
        curl https://packages.microsoft.com/config/ubuntu/16.04/prod.list > /etc/apt/sources.list.d/mssql-release.list
        apt-get update
        ACCEPT_EULA=Y apt-get install msodbcsql
        ACCEPT_EULA=Y apt-get install mssql-tools
        echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.bash_profile
        echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.bashrc
        source ~/.bashrc
    fi

elif [ -n "$AMAZON" ] && [ $AMAZON -gt 2017 ]; then
    printf "${SetColorToYELLOW}Checking dependencies...${SetNoColor}\n"
    yum groups install -y development
    yum groups install -y development-libs
    yum install -y clang
    wget https://cmake.org/files/v3.8/cmake-3.8.2.tar.gz
    tar -xf cmake-3.8.2.tar.gz
    cd cmake-3.8.2
    env CC=clang CXX=clang++ ./bootstrap
    make && make install
    export PATH=$PATH:/usr/local/bin
    cd ..
    rm -rf ./cmake*

else
    echo OS UNSUPPORTED
    exit
fi

# # #
# INSTALL VCPKG
#
cd /opt
{ ls vcpkg || git clone https://github.com/Microsoft/vcpkg; } &> /dev/null
cd vcpkg 
if [ ! -f ./vcpkg ]; then
    printf "${SetColorToYELLOW}Installing vcpkg...${SetNoColor}\n"
    ./bootstrap-vcpkg.sh
fi

printf "${SetColorToYELLOW}Checking dependencies packages...${SetNoColor}\n"
./vcpkg install boost-lockfree boost-regex poco rapidxml sqlite3
cd ..

# # #
# INSTALL BOOST
#
# boostVersion='1.66.0'
# boostLabel="boost_1_66_0"
# wget "https://dl.bintray.com/boostorg/release/$boostVersion/source/$boostLabel.tar.bz2"
# tar -xf "$boostLabel.tar.bz2"
# cd "$boostLabel/tools/build/"
# ./bootstrap.sh
# cd ../../
# ./tools/build/b2 -j 3 variant=debug link=static threading=multi toolset=clang runtime-link=shared --layout=tagged
# ./tools/build/b2 -j 3 variant=release link=static threading=multi toolset=clang runtime-link=shared --layout=tagged
# BOOST_HOME="/opt/boost-$boostVersion"
# mkdir $BOOST_HOME
# mkdir $BOOST_HOME/include
# mkdir $BOOST_HOME/lib
# mv stage/lib/* $BOOST_HOME/lib/
# mv boost $BOOST_HOME/include/
# cd ..
# rm -rf $boostLabel
# rm "$boostLabel.tar.bz2"

# # #
# INSTALL POCO
#
# pocoLabel='poco-1.9.0'
# pocoTarFile=$pocoLabel"-all.tar.gz"
# pocoXDir=$pocoLabel"-all"
# wget "https://pocoproject.org/releases/poco-1.9.0/$pocoTarFile"
# tar -xf $pocoTarFile
# cd $pocoXDir
# ./configure --omit=Data/MySQL,MongoDB,PageCompiler --config=Linux-clang --static --no-tests --no-samples --prefix="/opt/$pocoLabel"
# make -s -j3 || exit
# make install
# cd ..
# rm -rf $pocoXDir
# rm $pocoTarFile
