# download fmt-6.2.1/ jemalloc-5.2.1/ spdlog-1.5.0/

export LD_LIBRARY_PATH=/home/weixingsun/perf/wsp_client/wsc_main/lib
FILE="main.cpp -o wsc"
#STA="-static -Wl,-Bdynamic,-lgcc_s,-Bstatic"
#STA="-static-libgcc -static-libstdc++ -Wl,-Bdynamic,-lc,-ldl,-lpthread,-lgcc_s,-Bstatic,-lssl,-lcrypto,-lz"
#DIR="-L/usr/local/ssl/lib -I/usr/local/ssl/lib/include"
DIR="-Llib -Iinclude"
JEM="-ljemalloc -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free"
LIB="-lboost_thread -lfmt -lssl -lcrypto -lboost_chrono -ldl"
LIB="$LIB -lprometheus-cpp-push -lprometheus-cpp-pull -lprometheus-cpp-core -lpthread -lz"
#OPT="-w -m64 -Wl,-z,muldefs -O3 -xCORE-AVX512 -Ofast -ffast-math -flto -mfpmath=sse -funroll-loops"
OPT="-Ofast -ffast-math -march=native " # -flto -Wall -Wextra -Wpedantic 
ALL="$FILE $OPT $DIR $LIB $JEM -static"
CXX="clang++ -Wall -pedantic -Wno-vla -g -Werror -std=c++14"
#CXX="g++"
echo "$CXX $ALL"
$CXX $ALL
#./main --url='ws://172.20.150.6:9080/ws/!bookTicker'
