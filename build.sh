# download fmt-6.2.1/ jemalloc-5.2.1/ spdlog-1.5.0/

export LD_LIBRARY_PATH=/home/weixingsun/perf/wsp_client/wsc_main/lib
#OPT="-DSPDLOG_FMT_EXTERNAL=1 -lboost_system -lpthread -lssl -lcrypto -lfmt -static"
FILE="main.cpp -o wsc"
#STA="-static -Wl,-Bdynamic,-lgcc_s,-Bstatic"
#STA="-Wl,-Bdynamic,-lgcc_s,-Bstatic"
#DIR="-L/usr/local/ssl/lib -I/usr/local/ssl/lib/include"
DIR="-Llib -Iinclude"
JEM="-ljemalloc -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free"
LIB="-lpthread -lboost_thread -lfmt -lssl -lcrypto -lboost_chrono -lz -ldl -static"  #-lrt
#LIB="-static -lboost_thread"
#OPT="-w -m64 -Wl,-z,muldefs -O3 -xCORE-AVX512 -Ofast -ffast-math -flto -mfpmath=sse -funroll-loops"
OPT="-Ofast -ffast-math -flto -flto -march=native"  # -Wall -Wextra -Wpedantic 
ALL="$FILE $STA $DIR $LIB $JEM $OPT -DSPDLOG_FMT_EXTERNAL=1 "
CXX="clang++"
#CXX="g++"
echo "$CXX $ALL"
$CXX $ALL
#./main --url='ws://172.20.150.6:9080/ws/!bookTicker'
