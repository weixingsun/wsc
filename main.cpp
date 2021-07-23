#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>
#include <websocketpp/client.hpp>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>
//#include <sys/sdt.h>
//#include <thread>
#include <iostream>
#include <string>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;

struct deflate_config : public websocketpp::config::asio_client {
    typedef deflate_config type;
    typedef asio_client base;
    typedef base::concurrency_type concurrency_type;
    typedef base::request_type request_type;
    typedef base::response_type response_type;
    typedef base::message_type message_type;
    typedef base::con_msg_manager_type con_msg_manager_type;
    typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;
    typedef base::alog_type alog_type;
    typedef base::elog_type elog_type;
    typedef base::rng_type rng_type;
    
    struct transport_config : public base::transport_config {
        typedef type::concurrency_type concurrency_type;
        typedef type::alog_type alog_type;
        typedef type::elog_type elog_type;
        typedef type::request_type request_type;
        typedef type::response_type response_type;
        typedef websocketpp::transport::asio::basic_socket::endpoint socket_type;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config> 
        transport_type;
        
    /// permessage_compress extension
    struct permessage_deflate_config {};
    typedef websocketpp::extensions::permessage_deflate::enabled <permessage_deflate_config> permessage_deflate_type;
};

bool DEBUG=false;
bool ZIP=true;
bool REC=true;
std::string URL="";
std::string PORT="";
int INTERVAL=1;
int CNT = 0;
int N = 1;
int R_CNT = 0;
boost::thread_group tg;
std::string IP="";
std::string METRIC_NAME="";
prometheus::Gauge* rate;
prometheus::Gauge* reconn;
prometheus::Gauge* lat99;
prometheus::Gauge* lat100;

std::string convertToString(char* a){
    std::string s = a;
    return s;
}
void get_ip() {
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);
    char addressBuffer[INET_ADDRSTRLEN];
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        std::string eth(ifa->ifa_name);
        if (eth.rfind("lo", 0)==0|| eth.rfind("vir", 0)==0){
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //printf("%s -> %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    IP=convertToString(addressBuffer);
    std::cout<<"IP: "<<IP<<std::endl;
}
void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
    //spdlog::info("hdl: {} message: {}", hdl.lock().get(), msg->get_payload());
    CNT++;
    if (DEBUG) {
        spdlog::info("MSG: {}", msg->get_payload());
    }
}
void on_open_tls(websocketpp::client<websocketpp::config::asio_tls_client> *c, websocketpp::connection_hdl hdl) {
    //spdlog::info("connection open: hdl {} ", hdl.lock().get());
}
void on_open_no_tls(websocketpp::client<websocketpp::config::asio_client> *c, websocketpp::connection_hdl hdl) {
    //spdlog::info("connection open: hdl {} ", hdl.lock().get());
}
void on_open_no_tls_zip(websocketpp::client<deflate_config> *c, websocketpp::connection_hdl hdl) {
    //spdlog::info("connection open: hdl {} ", hdl.lock().get());
}
static context_ptr on_tls_init() {
    // establishes a SSL connection
    context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::single_dh_use);
    } catch (std::exception &e) {
        //spdlog::error("Error in context pointer: {}", e.what());
        //spdlog::error("Error in context pointer: {}", e.what());
    }
    return ctx;
}
void books(){
    websocketpp::client<websocketpp::config::asio_tls_client> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none);
        c.init_asio();
        c.set_tls_init_handler(bind(&on_tls_init));
        c.set_open_handler(bind(&on_open_tls, &c, ::_1));
        c.set_message_handler(bind(&on_message, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<websocketpp::config::asio_tls_client>::connection_ptr conn = c.get_connection(URL, ec);
        if (ec) {
            spdlog::error("could not create connection because: {}", ec.message());
            return;
        }
        c.connect(conn);
        c.run();
    } catch (websocketpp::exception const &e) {
        std::cout << e.what() << std::endl;
        spdlog::error("Books error: {}", e.what());
    }
}
void book(){
    websocketpp::client<websocketpp::config::asio_client> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none); //frame_payload
        c.init_asio();
        c.set_open_handler(bind(&on_open_no_tls, &c, ::_1));
        c.set_message_handler(bind(&on_message, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<websocketpp::config::asio_client>::connection_ptr conn = c.get_connection(URL, ec);
        if (ec) {
            spdlog::error("could not create connection because: {}", ec.message());
            return;
        }
        c.connect(conn);
        c.run();
    } catch (websocketpp::exception const &e) {
        std::cout << e.what() << std::endl;
        spdlog::error("Book error: {}", e.what());
    }
}
void bookz(){
    websocketpp::client<deflate_config> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none); //frame_payload
        c.init_asio();
        c.set_open_handler(bind(&on_open_no_tls_zip, &c, ::_1));
        c.set_message_handler(bind(&on_message, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<deflate_config>::connection_ptr conn = c.get_connection(URL, ec);
        if (ec) {
            spdlog::error("could not create connection because: {}", ec.message());
            return;
        }
        c.connect(conn);
        // for (int i = 0; i < N; i++) {
        //     websocketpp::lib::thread t1(websocketpp::client<deflate_config>::run, &c);
        //     t1.join()
        //     boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        // }
        c.run();
    } catch (websocketpp::exception const &e) {
        //std::cout << e.what() << std::endl;
        //spdlog::error("Bookz error: {}", e.what());
    }
}
bool loop(){
    for ( ;; ) {
        try {
            if (URL.rfind("wss", 0) == 0){
                books();
            }else if (ZIP) {
                bookz();
            }else{
                book();
            }
            if (REC==false){
                return true;
            }
        }catch ( ... ) {
            //if ( yes ) return false;
        }
        //std::cout << "reconnect (" <<DURATION<<")"<< std::endl;
        R_CNT++;
    }
}
void prom_init(){
    prometheus::Exposer exposer{"0.0.0.0:"+PORT};
    auto registry = std::make_shared<prometheus::Registry>();
    auto metric_name="WSC";
    auto metric_help="WS Client Performance Metrics";
    //auto& wsc_perf = BuildCounter().Name(metric_name).Help(metric_help).Register(*registry);
    auto& wsc_perf = prometheus::BuildGauge().Name(metric_name).Help(metric_help).Register(*registry);
    rate   = &wsc_perf.Add({{"type", "guage"}, {"name", "rate"+METRIC_NAME}});  //{"host", IP}, 
    reconn = &wsc_perf.Add({{"type", "guage"}, {"name", "reconn"+METRIC_NAME}});
    lat99  = &wsc_perf.Add({{"type", "guage"}, {"name", "lat99"+METRIC_NAME}});
    lat100 = &wsc_perf.Add({{"type", "guage"}, {"name", "lat100"+METRIC_NAME}});
    exposer.RegisterCollectable(registry);
}
void prom_upload(){
    rate->Set(CNT);
    reconn->Set(R_CNT);
    lat99->Set(0);
    lat100->Set(0);
}
void print() {
    while(true){ //std::chrono::steady_clock::now()
        boost::this_thread::sleep_for(boost::chrono::seconds(INTERVAL));
        spdlog::info("Reconnect[{}] MSG Rate: {}/s", R_CNT,CNT);
        CNT=0;
        prom_upload();
    }
}

void threads_boost() {
    for (int i = 0; i < N; i++) {
        tg.create_thread(loop);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }
    tg.create_thread(print);
    tg.join_all();
}
int nthSubstr(int n, const std::string& s, const std::string& p) {
   std::string::size_type i = s.find(p);     // first index
   int j=0;
   for (j = 1; j < n && i != std::string::npos; ++j)
      i = s.find(p, i+1); // Find the next index
   if (j == n) return(i);
   else return(-1);
}
//"wss://stream.binance.com:9443/ws/btcusdt@bookTicker";
//"wss://stream.binance.com:9443/ws/!bookTicker";
//"ws://172.20.150.230:9080/ws/!bookTicker";
//"ws://172.20.150.230:9080/ws/btc001@bookTicker/btc002@bookTicker/btc003@bookTicker/btc004@bookTicker/btc005@bookTicker"
void append_url(std::string SYMBOLS, std::string STREAMS){
    std::vector<std::string> symbolv;
    std::vector<std::string> streamv;
    std::stringstream ss(SYMBOLS);
    std::stringstream sm(STREAMS);
    while(ss.good()) {
        std::string substr;
        getline(ss, substr, ',');
        if (substr.size()>0) symbolv.push_back(substr);
    }
    while(sm.good()) {
        std::string substr;
        getline(sm, substr, ',');
        if (substr.size()>0) streamv.push_back(substr);
    }
    if (DEBUG){
        std::cout<<"SYMBOLS="<<SYMBOLS<<"STREAMS="<<STREAMS<<std::endl;
        std::cout<<"symbolv="<<symbolv.size()<<"streamv="<<streamv.size()<<std::endl;
    }
    for(int i = 0; i<streamv.size(); i++) {
        auto stream = streamv.at(i);
        for(int i = 0; i<symbolv.size(); i++) {
            auto sym = symbolv.at(i);
            auto part = "/"+sym+"@"+stream;
            URL+=part;
        }
    }
    if (streamv.size()>1){
        METRIC_NAME="@mixed";
    }else if (URL.find("!") != std::string::npos) {
        METRIC_NAME="!bookTicker";
    }else if (URL.find("@") != std::string::npos) {
        int start = nthSubstr(1, URL, "@");
        int end = nthSubstr(5, URL, "/");
        METRIC_NAME=URL.substr (start,end-start);
    }
    
    std::cout<<URL<<std::endl;
}
int main(int argc, char** argv) {
    cxxopts::Options options("wsclient", "websocket client for binance stream");
    options.add_options()
        ("d,debug",   "Enable debugging",   cxxopts::value<bool>()->default_value("false"))
        ("v,verbose", "Verbose output",     cxxopts::value<bool>()->default_value("false"))
        ("z,zlib",    "Permessage Deflate", cxxopts::value<bool>()->default_value("true"))
        ("r,reconn",  "Reconnect on error", cxxopts::value<bool>()->default_value("true"))
        ("u,url",     "URL",                cxxopts::value<std::string>()->default_value("wss://stream.binance.com:9443/ws/!bookTicker"))
        ("s,symbols", "Symbols",            cxxopts::value<std::string>()->default_value(""))  //btcusdt,ethusdt
        ("m,streams", "Streams",            cxxopts::value<std::string>()->default_value(""))  //bookTicker
        ("t,threads", "Threads",            cxxopts::value<int>()->default_value("1"))
        //("n,duration","Duration",         cxxopts::value<int>()->default_value("60"))
        ("i,interval","Interval",           cxxopts::value<int>()->default_value("1"))
        ("f,freq",    "Sampling Frequency", cxxopts::value<int>()->default_value("1000")) //sampling every N msg
        ("p,promport","Prometheus Port",    cxxopts::value<std::string>()->default_value("8080"))
        ("h,help",    "Print usage")
    ;
    auto result = options.parse(argc, argv);
    if (result.count("help")){
      std::cout << options.help() << std::endl;
      exit(0);
    }
    DEBUG = result["debug"].as<bool>();
    URL=result["url"].as<std::string>();
    INTERVAL=result["interval"].as<int>();
    N=result["threads"].as<int>();
    REC=result["reconn"].as<bool>();
    PORT=result["promport"].as<std::string>();
    std::cout<<"Prometheus Port: "<<PORT<<std::endl;
    //DURATION=result["duration"].as<int>();
    append_url(result["symbols"].as<std::string>(),result["streams"].as<std::string>());
    get_ip();
    prom_init();
    threads_boost();
    return 0;
}
