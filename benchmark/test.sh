#!/bin/zsh

# get two arguments

experiment=$1
file=$2
threads=$3
degree=$4
sort=$5

# basic files
cp ../mainTest.cpp main.cpp
cp ../Set4Batches.hpp .

sed -i.bak "s/{{EXPERIMENT}}/${experiment}/g" main.cpp
sed -i.bak "s/{{FILE}}/${file}/g" main.cpp
sed -i.bak "s/{{THREADS}}/${threads}/g" main.cpp
sed -i.bak "s/{{DEGREE}}/${degree}/g" main.cpp
sed -i.bak "s/{{SORT}}/${sort}/g" main.cpp

# compile

/opt/homebrew/bin/g++-11 -lpthread main.cpp -o main > /dev/null

# execute

./main

# clean

rm main
rm main.cpp
rm Set4Batches.hpp
rm main.cpp.bak
