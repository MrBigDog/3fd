#!/bin/bash

UBUNTU=$(cat /etc/issue | grep -i ubuntu | awk -F" " '{print $2}' | awk -F"." '{print $1}')
AMAZON=$(cat /etc/issue | grep -i "amazon linux ami release" | awk -F" " '{print $5}' | awk -F"." '{print $1}')

export SetColorToYELLOW='\033[0;33m';
export SetNoColor='\033[0m';

# # #
# SETUP DEV ENV
#

if [ -n "$UBUNTU" ] && [ $UBUNTU -gt 14 ]; then

    if [ -z $CXX ] && [ -z "$(dpkg -l | grep 'g++')" ]; then
        printf "${SetColorToYELLOW}Installing GNU C++ compiler...${SetNoColor}\n"
        sudo apt install --assume-yes g++
    fi

    printf "${SetColorToYELLOW}Checking dependencies...${SetNoColor}\n"
    sudo apt install --assume-yes cmake unixodbc unixodbc-dev openssl libssl-dev

    HAS_MSSQL_TOOLS=$(dpkg -l | grep "mssql-tools")

    # MSSQL TOOLS not yet installed?
    if [ -z "$HAS_MSSQL_TOOLS" ]; then
        printf "${SetColorToYELLOW}Installing MSSQL tools...${SetNoColor}\n"
        curl https://packages.microsoft.com/keys/microsoft.asc | sudo apt-key add -
        sudo curl https://packages.microsoft.com/config/ubuntu/16.04/prod.list > /etc/apt/sources.list.d/mssql-release.list
        sudo apt-get update
        ACCEPT_EULA=Y sudo apt-get install msodbcsql
        ACCEPT_EULA=Y sudo apt-get install mssql-tools
        echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.bash_profile
        echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.bashrc
        source ~/.bashrc
    fi

elif [ -n "$AMAZON" ] && [ $AMAZON -gt 2017 ]; then
    printf "${SetColorToYELLOW}Checking dependencies...${SetNoColor}\n"
    yum groups install -y development
    yum groups install -y development-libs
    wget https://cmake.org/files/v3.10/cmake-3.10.3.tar.gz
    tar -xf cmake-3.10.3.tar.gz
    cd cmake-3.10.3
    ./bootstrap
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
# cd /opt
# { ls vcpkg || git clone https://github.com/Microsoft/vcpkg; } &> /dev/null
# cd vcpkg 
# if [ ! -f ./vcpkg ]; then
#     printf "${SetColorToYELLOW}Installing vcpkg...${SetNoColor}\n"
#     ./bootstrap-vcpkg.sh
# fi
#
# printf "${SetColorToYELLOW}Checking dependencies packages...${SetNoColor}\n"
# ./vcpkg install boost-lockfree boost-regex poco rapidxml sqlite3
# cd ..

if [ -d build ]; then
    printf "${SetColorToYELLOW}Do you wish to download and rebuild Boost and POCO C++ frou source?${SetNoColor}"
    while true; do
        read -p " [yes/no] " yn
        case $yn in
            [Yy]* ) rm -rf build;;
            [Nn]* ) exit;;
            * ) echo "Please answer yes or no.";;
        esac
    done
fi

mkdir build && cd build

numCpuCores=$(grep -c ^processor /proc/cpuinfo)

# # #
# INSTALL BOOST
#
printf "${SetColorToYELLOW}Downloading Boost source...${SetNoColor}\n"
boostVersion='1.67.0'
boostLabel="boost_1_67_0"
wget "https://dl.bintray.com/boostorg/release/$boostVersion/source/$boostLabel.tar.gz"
printf "${SetColorToYELLOW}Unpacking Boost source...${SetNoColor}\n"
tar -xf "$boostLabel.tar.gz"
printf "${SetColorToYELLOW}Building Boost...${SetNoColor}\n"
cd $boostLabel
./bootstrap.sh --prefix=../ --with-libraries=system,thread,regex
./b2 -j $numCpuCores variant=debug link=static threading=multi runtime-link=shared --layout=tagged install
./b2 -j $numCpuCores variant=release link=static threading=multi runtime-link=shared --layout=tagged install
cd ..
rm -rf $boostLabel
rm "$boostLabel.tar.gz"

# # #
# INSTALL POCO
#
printf "${SetColorToYELLOW}Downloading POCO C++ libs source...${SetNoColor}\n"
pocoLabel='poco-1.9.0'
pocoTarFile=$pocoLabel"-all.tar.gz"
pocoXDir=$pocoLabel"-all"
wget "https://pocoproject.org/releases/poco-1.9.0/$pocoTarFile"
printf "${SetColorToYELLOW}Unpacking POCO C++ libs source...${SetNoColor}\n"
tar -xf $pocoTarFile
cd $pocoXDir
printf "${SetColorToYELLOW}Building POCO C++ libs...${SetNoColor}\n"
./configure --omit=Data/MySQL,Data/SQLite,Dynamic,JSON,MongoDB,PageCompiler,Redis --static --no-tests --no-samples --prefix=../
make -s -j $numCpuCores || exit
make install
cd ..
rm -rf $pocoXDir
rm $pocoTarFile

