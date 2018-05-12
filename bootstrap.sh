#!/bin/bash

UBUNTU=$(cat /etc/issue | grep -i ubuntu | awk -F" " '{print $2}' | awk -F"." '{print $1}')
AMAZON=$(cat /etc/issue | grep -i "amazon linux ami release" | awk -F" " '{print $5}' | awk -F"." '{print $1}')

export SetColorToYELLOW='\033[0;33m';
export SetNoColor='\033[0m';

# # #
# INSTALL MSSQL TOOLS & ODBC DRIVER
#
function installMsSqlOdbc()
{
    printf "${SetColorToYELLOW}Installing MSSQL tools...${SetNoColor}\n"
    curl http://packages.microsoft.com/keys/microsoft.asc | sudo apt-key add -
    sudo curl https://packages.microsoft.com/config/ubuntu/16.04/prod.list > /etc/apt/sources.list.d/mssql-release.list
    sudo apt-get update
    ACCEPT_EULA=Y sudo apt install --assume-yes msodbcsql
    ACCEPT_EULA=Y sudo apt install --assume-yes mssql-tools

    if [ -f $HOME/.bash_profile ] && [ -z "$(cat $HOME/.bash_profile | grep '$PATH:/opt/mssql-tools/bins')" ];
    then
        echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.bash_profile
    fi

    if [ -f $HOME/.profile ] && [ -z "$(cat $HOME/.profile | grep '$PATH:/opt/mssql-tools/bins')" ];
    then
        echo 'export PATH="$PATH:/opt/mssql-tools/bin"' >> ~/.profile
    fi

    export PATH="$PATH:/opt/mssql-tools/bin"
}

# # #
# INSTALL PACKAGES & TOOLCHAIN
#

if [ -n "$UBUNTU" ] && [ $UBUNTU -gt 14 ];
then

    if [ -z $CXX ] && [ -z "$(dpkg -l | grep 'g++')" ];
    then
        printf "${SetColorToYELLOW}Installing GNU C++ compiler...${SetNoColor}\n"
        sudo apt install --assume-yes g++
    fi

    printf "${SetColorToYELLOW}Checking dependencies...${SetNoColor}\n"
    sudo apt install --assume-yes unzip cmake unixodbc unixodbc-dev openssl libssl-dev

    HAS_MSSQL_TOOLS=$(dpkg -l | grep "mssql-tools")

    # MSSQL TOOLS not yet installed?
    if [ -z "$HAS_MSSQL_TOOLS" ];
    then
        printf "${SetColorToYELLOW}Do you wish to install Microsoft SQL Server ODBC driver?${SetNoColor}"
        while true; do
            read -p " [yes/no] " yn
            case $yn in
                [Yy]* ) installMsSqlOdbc
                        break;;
                [Nn]* ) break;;
                * ) printf "Please answer yes or no.";;
            esac
        done
    fi

elif [ -n "$AMAZON" ] && [ $AMAZON -gt 2017 ];
then
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
# if [ ! -f ./vcpkg ];
# then
#     printf "${SetColorToYELLOW}Installing vcpkg...${SetNoColor}\n"
#     ./bootstrap-vcpkg.sh
# fi
#
# printf "${SetColorToYELLOW}Checking dependencies packages...${SetNoColor}\n"
# ./vcpkg install boost-lockfree boost-regex poco rapidxml sqlite3
# cd ..

{ ls build || mkdir build; } &> /dev/null
cd build

# # #
# BUILD RAPIDXML
#
function buildRapidxml()
{
    find . | grep 'rapidxml' | xargs rm -rf
    RAPIDXML='rapidxml-1.13'
    wget "https://netcologne.dl.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/${RAPIDXML}.zip"
    unzip "${RAPIDXML}.zip"
    mv $RAPIDXML/*.hpp include/
    rm -rf $RAPIDXML*
}

function buildSqlite3()
{
    find . | grep 'sqlite' | xargs rm -rf
    SQLITE='sqlite-autoconf-3230100'
    wget "http://sqlite.org/2018/${SQLITE}.tar.gz"
    tar -xf "${SQLITE}.tar.gz"
    INSTALLDIR=$(pwd)
    cd $SQLITE
    printf "${SetColorToYELLOW}Building SQLite3 as static library (release)...${SetNoColor}\n"
    ./configure --enable-shared=no --prefix=$INSTALLDIR
    make && make install
    cd ..
    rm -rf $SQLITE*
}

if [ -d include ]; then
    printf "${SetColorToYELLOW}Do you wish to download and (re)build RAPIDXML and SQLITE3 from source?${SetNoColor}"
    while true; do
        read -p " [yes/no] " yn
        case $yn in
            [Yy]* ) buildRapidxml
                    buildSqlite3
                    break;;
            [Nn]* ) break;;
            * ) printf "Please answer yes or no.";;
        esac
    done
else
    buildRapidxml
    buildSqlite3
fi

numCpuCores=$(grep -c ^processor /proc/cpuinfo)

# # #
# BUILD BOOST
#
function buildBoost()
{
    find ./lib | grep boost | xargs rm
    { ls include/boost && rm -rf include/boost; } &> /dev/null
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
}

if [ -d include/boost ]; then
    printf "${SetColorToYELLOW}Do you wish to download and (re)build Boost library dependencies from source?${SetNoColor}"
    while true; do
        read -p " [yes/no] " yn
        case $yn in
            [Yy]* ) buildBoost
                    break;;
            [Nn]* ) break;;
            * ) printf "Please answer yes or no.";;
        esac
    done
else
    buildBoost
fi

# # #
# BUILD POCO
#
function buildPoco()
{
    find ./lib | grep Poco | xargs rm
    { ls include/Poco && rm -rf include/Poco; } &> /dev/null
    printf "${SetColorToYELLOW}Downloading POCO C++ libs source...${SetNoColor}\n"
    pocoLabel='poco-1.9.0'
    pocoTarFile=$pocoLabel"-all.tar.gz"
    pocoXDir=$pocoLabel"-all"
    wget "https://pocoproject.org/releases/${pocoLabel}/${pocoTarFile}"
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
}

if [ -d include/Poco ]; then
    printf "${SetColorToYELLOW}Do you wish to download and (re)build POCO C++ library dependencies from source?${SetNoColor}"
    while true; do
        read -p " [yes/no] " yn
        case $yn in
            [Yy]* ) buildPoco
                    break;;
            [Nn]* ) break;;
            * ) printf "Please answer yes or no.";;
        esac
    done
else
    buildPoco
fi
