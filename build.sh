#!/bin/bash

{ ls build || printf 'Run configure.sh first!'; } &> /dev/null

cd gtest
make
cd ../3FD
make install
cd ../UnitTests
make install
cd ../IntegrationTests
make install
cd ../
find 3FD/* | grep -v '_impl' | grep '\.h$' | xargs -I{} cp {} build/include/3FD/
cp -rf btree  build/include/
cp -rf OpenCL build/include/
cp CreateMsSqlSvcBrokerDB.sql build/
cp Acknowledgements.txt       build/
cp LICENSE build/
cp README  build/
