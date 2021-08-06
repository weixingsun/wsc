#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>
#include <websocketpp/client.hpp>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <boost/chrono.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>
#include <prometheus/gauge.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <jemalloc/jemalloc.h>
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
//#include <sys/sdt.h>
//#include <thread>
//#include <ifaddrs.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>

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

    typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;
        
    /// permessage_compress extension
    struct permessage_deflate_config {
        typedef core_client::request_type request_type;
        static const bool allow_disabling_context_takeover = true;
        static const uint8_t minimum_outgoing_window_bits = 8; //client_max_window_bits (8-15)
    };
    typedef websocketpp::extensions::permessage_deflate::enabled <permessage_deflate_config> permessage_deflate_type;
    //static const size_t max_message_size = 16000000;
    //static const bool enable_extensions = true;
};
struct deflate_config_15 : public websocketpp::config::asio_client {
    typedef deflate_config_15 type;
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

    typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;
        
    /// permessage_compress extension
    struct permessage_deflate_config {
        typedef core_client::request_type request_type;
        static const bool allow_disabling_context_takeover = true;
        static const uint8_t minimum_outgoing_window_bits = 15; //client_max_window_bits (8-15)
    };
    typedef websocketpp::extensions::permessage_deflate::enabled <permessage_deflate_config> permessage_deflate_type;
    //static const size_t max_message_size = 16000000;
    //static const bool enable_extensions = true;
};

bool ZIP=true;
bool REC=true;
bool TIMED=true;
std::string URL="";
std::string PORT="";
int WIN_BITS=8;
int INTERVAL=1;
int R_CNT = 0;
int N = 1;
int CNT = 0;
int T_CNT = 0;
int T_SAMPLE = 1000;
int TC_N = 0;        // Timed Connection Running Counter
double LAT_MAX = 0;
double LAT_SUM = 0;  // not an array/vector since worse perf
int LAT_CNT = 0;
const std::string JSON=R"({"method":"SET_PROPERTY","params":["timed",true],"id":1})";
boost::thread_group tg;
//std::string IP="";
std::string METRIC_NAME="";
prometheus::Gauge* rate;
prometheus::Gauge* reconn;
prometheus::Gauge* latavg;
prometheus::Gauge* latmax;

std::string convertToString(char* a){
    std::string s = a;
    return s;
}
void send_timed(websocketpp::client<websocketpp::config::asio_client> *c, websocketpp::connection_hdl hdl){
    c->send(hdl, JSON, websocketpp::frame::opcode::text);
}
void ssend_timed(websocketpp::client<websocketpp::config::asio_tls_client> *c, websocketpp::connection_hdl hdl){
    c->send(hdl, JSON, websocketpp::frame::opcode::text);
}
void zsend_timed(websocketpp::client<deflate_config> *c, websocketpp::connection_hdl hdl){
    c->send(hdl, JSON, websocketpp::frame::opcode::text);
}
void z15_send_timed(websocketpp::client<deflate_config_15> *c, websocketpp::connection_hdl hdl){
    c->send(hdl, JSON, websocketpp::frame::opcode::text);
}

void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
    //spdlog::info("hdl: {} message: {}", hdl.lock().get(), msg->get_payload());
    CNT++;
    spdlog::debug("MSG: {}",msg->get_raw_payload());
    //{"u":162673419,"s":"HBARBTC","b":"0.00000499","B":"5579.00000000","a":"0.00000500","A":"20621.00000000"}
}
void on_message_timed(websocketpp::connection_hdl hdl, message_ptr msg) {
    CNT++;
    T_CNT++;
    //{"stream":"!bookTicker","data":{"u":152478534,"s":"XVGETH","b":"0.917","B":"40479.0","a":"0.919","A":"17881.0"},"V":1627380845283,"Y":1627380845283,"W":1627380845283}
    if (T_CNT%T_SAMPLE==0){
        //std::chrono::steady_clock //system_clock
        const std::string& payload = msg->get_raw_payload();
        std::size_t start = payload.rfind(R"("W":)");
        spdlog::debug("MSG: {}",msg->get_raw_payload());
        //spdlog::debug("T_CNT={} T_SAMPLE={} LAT_CNT={} ",T_CNT,T_SAMPLE,LAT_CNT);
        if (start!=std::string::npos){
            LAT_CNT++;
            std::string ws = payload.substr(start+4,13);
            //unsigned long long W = std::stoull(ws); //msg->get_payload().W;
            //unsigned long now = boost::chrono::steady_clock::now().time_since_epoch() / boost::chrono::milliseconds(1);
            unsigned long W = std::stoull(ws);
            unsigned long now = boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();
            //std::cout<<now<<"-"<<W<<std::endl;
            unsigned long lat=now-W;
            LAT_SUM+=lat;
            if (LAT_MAX<lat) LAT_MAX=lat;
        }
    }
}
void on_open_tls(websocketpp::client<websocketpp::config::asio_tls_client> *c, websocketpp::connection_hdl hdl) {
    //spdlog::info("connection open: hdl {} ", hdl.lock().get());
    spdlog::debug("TLS Connection: opened");
    ssend_timed(c,hdl);
}
void on_open_no_tls(websocketpp::client<websocketpp::config::asio_client> *c, websocketpp::connection_hdl hdl) {
    //spdlog::info("connection open: hdl {} ", hdl.lock().get());
    spdlog::debug("Non-TLS Connection: opened");
    send_timed(c,hdl);
}
void on_open_no_tls_zip(websocketpp::client<deflate_config> *c, websocketpp::connection_hdl hdl) {
    //spdlog::info("connection open: hdl {} ", hdl.lock().get());
    spdlog::debug("Zip Non-TLS Connection: opened");
    zsend_timed(c,hdl);
}
void on_open_no_tls_zip15(websocketpp::client<deflate_config_15> *c, websocketpp::connection_hdl hdl) {
    //spdlog::info("connection open: hdl {} ", hdl.lock().get());
    spdlog::debug("Zip15 Non-TLS Connection: opened");
    z15_send_timed(c,hdl);
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
        //c.set_open_handler(bind(&on_open_tls, &c, ::_1));
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
void bookst(){
    websocketpp::client<websocketpp::config::asio_tls_client> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none);
        c.init_asio();
        c.set_tls_init_handler(bind(&on_tls_init));
        c.set_open_handler(bind(&on_open_tls, &c, ::_1));
        c.set_message_handler(bind(&on_message_timed, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<websocketpp::config::asio_tls_client>::connection_ptr conn = c.get_connection(URL, ec);
        conn->append_header("User-Agent","WSC-perf v1");
        conn->append_header("X-Tracky-ID",boost::lexical_cast<std::string>(boost::uuids::random_generator()()));
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
        //c.set_open_handler(bind(&on_open_no_tls, &c, ::_1));
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
void bookt(){
    websocketpp::client<websocketpp::config::asio_client> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none); //frame_payload
        c.init_asio();
        c.set_open_handler(bind(&on_open_no_tls, &c, ::_1));
        c.set_message_handler(bind(&on_message_timed, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<websocketpp::config::asio_client>::connection_ptr conn = c.get_connection(URL, ec);
        conn->append_header("User-Agent","WSC-perf v1");
        conn->append_header("X-Tracky-ID",boost::lexical_cast<std::string>(boost::uuids::random_generator()()));
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
        //c.set_open_handler(bind(&on_open_no_tls_zip, &c, ::_1));
        c.set_message_handler(bind(&on_message, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<deflate_config>::connection_ptr conn = c.get_connection(URL, ec);
        //const std::string header = conn->get_request_header();
        //std::cout<<"header: "<<header<<std::endl;
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
void bookz15(){
    websocketpp::client<deflate_config_15> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none); //frame_payload
        c.init_asio();
        //c.set_open_handler(bind(&on_open_no_tls_zip, &c, ::_1));
        c.set_message_handler(bind(&on_message, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<deflate_config_15>::connection_ptr conn = c.get_connection(URL, ec);
        //const std::string header = conn->get_request_header();
        //std::cout<<"header: "<<header<<std::endl;
        if (ec) {
            spdlog::error("could not create connection because: {}", ec.message());
            return;
        }
        c.connect(conn);
        c.run();
    } catch (websocketpp::exception const &e) {
        //std::cout << e.what() << std::endl;
        //spdlog::error("Bookz error: {}", e.what());
    }
}
void bookzt(){
    websocketpp::client<deflate_config> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none); //frame_payload
        c.init_asio();
        c.set_open_handler(bind(&on_open_no_tls_zip, &c, ::_1));
        c.set_message_handler(bind(&on_message_timed, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<deflate_config>::connection_ptr conn = c.get_connection(URL, ec);
        //std::cout<<"Timed Connection Running: "<<TC_RN<<std::endl;
        conn->append_header("User-Agent","WSC-perf v1");
        conn->append_header("X-Tracky-ID",boost::lexical_cast<std::string>(boost::uuids::random_generator()()));
        //const std::string header = conn->get_request_header();
        //std::cout<<"header: "<<header<<std::endl;
        if (ec) {
            spdlog::error("could not create timed connection because: {}", ec.message());
            return;
        }
        c.connect(conn);
        c.run();
    } catch (websocketpp::exception const &e) {
        //std::cout << e.what() << std::endl;
        //spdlog::error("Bookz error: {}", e.what());
    }
}
void bookz15t(){
    websocketpp::client<deflate_config_15> c;
    try {
        c.set_access_channels(websocketpp::log::alevel::none);
        c.clear_access_channels(websocketpp::log::alevel::none); //frame_payload
        c.init_asio();
        c.set_open_handler(bind(&on_open_no_tls_zip15, &c, ::_1));
        c.set_message_handler(bind(&on_message_timed, ::_1, ::_2));
        websocketpp::lib::error_code ec;
        websocketpp::client<deflate_config_15>::connection_ptr conn = c.get_connection(URL, ec);
        //std::cout<<"Timed Connection Running: "<<TC_RN<<std::endl;
        conn->append_header("User-Agent","WSC-perf v1");
        conn->append_header("X-Tracky-ID",boost::lexical_cast<std::string>(boost::uuids::random_generator()()));
        //const std::string header = conn->get_request_header();
        //std::cout<<"header: "<<header<<std::endl;
        if (ec) {
            spdlog::error("could not create timed connection because: {}", ec.message());
            return;
        }
        c.connect(conn);
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
                if (WIN_BITS>8) bookz15();
                else bookz();
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
bool loopt(){
    for ( ;; ) {
        try {
            if (URL.rfind("wss", 0) == 0){
                bookst();
            }else if (ZIP) {
                if (WIN_BITS>8) bookz15t();
                else bookzt();
            }else{
                bookt();
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
void prom_upload(){
    if (rate){
        rate->Set(CNT/INTERVAL);
        CNT=0;
        T_CNT=0;
        reconn->Set(R_CNT);
        R_CNT=0;
        if (LAT_CNT>0){
            latavg->Set(LAT_SUM/LAT_CNT);
            latmax->Set(LAT_MAX);
            LAT_CNT=0;
        }
        LAT_SUM=0;
        LAT_MAX=0;
    }
}
void print() {
    while(true){
        boost::this_thread::sleep_for(boost::chrono::seconds(INTERVAL));
        double Lavg=0;
        if (LAT_CNT>0){
            Lavg=LAT_SUM/LAT_CNT;
        }
        spdlog::warn("Reconn[{}] LAT[{}] {:03.2f} - {:03.2f} ms. Rate: {} /s ",R_CNT,LAT_CNT,Lavg,LAT_MAX,(int)(CNT/INTERVAL));
        prom_upload();
    }
}
void prom_init(){
    prometheus::Exposer exposer{PORT};
    auto registry = std::make_shared<prometheus::Registry>();
    auto& wsc_rate   = prometheus::BuildGauge().Name("wsc_client_rate")  .Help("").Labels({{"job","perf_qa_job"}}).Register(*registry);
    auto& wsc_latavg = prometheus::BuildGauge().Name("wsc_client_latavg").Help("").Labels({{"job","perf_qa_job"}}).Register(*registry);
    auto& wsc_latmax = prometheus::BuildGauge().Name("wsc_client_latmax").Help("").Labels({{"job","perf_qa_job"}}).Register(*registry);
    auto& wsc_reconn = prometheus::BuildGauge().Name("wsc_client_reconn").Help("").Labels({{"job","perf_qa_job"}}).Register(*registry);
    //rate    = &wsc_perf.Add({{"type", "guage"}, {"name", "rate"  +METRIC_NAME}});  //{"host", IP},
    //reconn  = &wsc_perf.Add({{"type", "guage"}, {"name", "reconn"+METRIC_NAME}});
    //latavg  = &wsc_perf.Add({{"type", "guage"}, {"name", "latavg"+METRIC_NAME}});
    //latmax  = &wsc_perf.Add({{"type", "guage"}, {"name", "latmax"+METRIC_NAME}});
    rate    = &wsc_rate.Add({});
    reconn  = &wsc_reconn.Add({});
    latavg  = &wsc_latavg.Add({});
    latmax  = &wsc_latmax.Add({});
    exposer.RegisterCollectable(registry);
    print();
}

void threads_boost() {
    for (int i = 0; i < TC_N; i++) {
        tg.create_thread(loopt);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }
    for (int i = TC_N; i < N; i++) {
        tg.create_thread(loop);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }
    //tg.create_thread(print);
    tg.create_thread(prom_init);
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
        ("g,debug",   "Enable debugging",   cxxopts::value<bool>()->default_value("false"))
        ("v,version", "Version output",     cxxopts::value<bool>()->default_value("false"))
        ("z,zlib",    "Permessage Deflate", cxxopts::value<bool>()->default_value("true"))
        ("r,reconn",  "Reconnect on error", cxxopts::value<bool>()->default_value("true"))
        ("l,lat",     "Timed Latency",      cxxopts::value<bool>()->default_value("false"))
        ("c,tcpct",   "Timed Conn Percentage", cxxopts::value<int>()->default_value("100"))
        ("d,tsmpl",   "Timed Sampling",     cxxopts::value<int>()->default_value("1000")) //sampling every N msg
        ("u,url",     "URL",                cxxopts::value<std::string>()->default_value("wss://stream.binance.com:9443/ws/!bookTicker"))
        ("s,symbols", "Symbols",            cxxopts::value<std::string>()->default_value(""))  //btcusdt,ethusdt
        ("m,streams", "Streams",            cxxopts::value<std::string>()->default_value(""))  //bookTicker
        ("t,threads", "Threads",            cxxopts::value<int>()->default_value("1"))
        ("i,interval","Interval",           cxxopts::value<int>()->default_value("1"))
        ("w,win_bits","Deflate Window Bits",cxxopts::value<int>()->default_value("8"))
        ("p,promport","Prometheus Port",    cxxopts::value<std::string>()->default_value("9338"))
        ("h,help",    "Print usage")
        //("n,duration","Duration",         cxxopts::value<int>()->default_value("60"))
    ;
    auto result = options.parse(argc, argv);
    if (result.count("help")){
      std::cout << options.help() << std::endl;
      exit(0);
    }
    if (result.count("version")){
      std::cout << "websocket client for binance stream\n" << "version: 0.0.1" << std::endl;
      exit(0);
    }
    bool DEBUG = result["debug"].as<bool>();
    if (DEBUG) spdlog::set_level(spdlog::level::debug);
    else spdlog::set_level(spdlog::level::warn);
    URL=result["url"].as<std::string>();
    INTERVAL=result["interval"].as<int>();
    N=result["threads"].as<int>();
    REC=result["reconn"].as<bool>();
    TIMED=result["lat"].as<bool>();
    PORT=result["promport"].as<std::string>();
    std::cout<<"Prometheus Port: "<<PORT<<std::endl;
    WIN_BITS=result["win_bits"].as<int>();
    if (WIN_BITS!=8 && WIN_BITS!=15){
        std::cout<<"Invalid Deflate Window Bits: "<<WIN_BITS<<std::endl;
        exit(0);
    }
    T_SAMPLE=result["tsmpl"].as<int>();
    int TC_PCT=result["tcpct"].as<int>();
    if (TIMED){
        TC_N=(int)(N*TC_PCT/100);
        if (TC_N<1) TC_N=1;
        std::cout<<"Timed Connection: "<<TC_N<<"/"<<N<<std::endl;
    }
    //DURATION=result["duration"].as<int>();
    append_url(result["symbols"].as<std::string>(),result["streams"].as<std::string>());
    //get_ip(); 
    std::cout<<"[TimeStamp] [warning] Re-connections[#] LAT[Samples] avg - max ms. Rate: msg / second"<<std::endl;
    threads_boost();
    return 0;
}