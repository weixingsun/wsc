#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>
#include <websocketpp/client.hpp>
#include "cxxopts.hpp"
#include <spdlog/spdlog.h>
//#include <sys/sdt.h>
//#include <thread>
#include <iostream>
#include <string>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

bool DEBUG=false;
bool ZIP=true;
std::string URL="";
std::string SYMBOL="";
int DURATION=10;
int INTERVAL=1;
int CNT = 0;
boost::thread_group tg;

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
        c.run();
    } catch (websocketpp::exception const &e) {
        std::cout << e.what() << std::endl;
        spdlog::error("Bookz error: {}", e.what());
    }
}
void books(){
    websocketpp::client<websocketpp::config::asio_tls_client> c;
    try {
        //c.set_access_channels(websocketpp::log::alevel::all);
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
            return true;
        }catch ( ... ) {
            std::cout << "reconnect (" <<DURATION<<")"<< std::endl;
            //if ( yes ) return false;
        }
    }
}

void print() {
    //auto end = std::chrono::steady_clock::now()+std::chrono::seconds(DURATION);
    while(true){ //std::chrono::steady_clock::now()<end
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1000*INTERVAL));
        spdlog::info("[{}] MSG Rate: {}/s", tg.size(),CNT);
        CNT=0;
        DURATION--;
    }
}

void threads_boost(int N) {
    std::chrono::milliseconds interval(10);
    for (int i = 0; i < N; i++) {
        tg.create_thread(loop);
    }
    tg.create_thread(print);
    tg.join_all();
}

//"wss://stream.binance.com:9443/ws/btcusdt@bookTicker";
//"wss://stream.binance.com:9443/ws/!bookTicker";
//"ws://172.20.150.6:9080/ws/!bookTicker";
//"ws://172.20.150.230:9080/ws/btc001@bookTicker/btc002@bookTicker/btc003@bookTicker/btc004@bookTicker/btc005@bookTicker"
int main(int argc, char** argv) {
    cxxopts::Options options("wsclient", "websocket client for binance stream");
    options.add_options()
        ("d,debug",   "Enable debugging",   cxxopts::value<bool>()->default_value("false"))
        ("v,verbose", "Verbose output",     cxxopts::value<bool>()->default_value("false"))
        ("z,zlib",    "Permessage Deflate", cxxopts::value<bool>()->default_value("true"))
        ("u,url",     "URL",                cxxopts::value<std::string>()->default_value("wss://stream.binance.com:9443/ws/!bookTicker"))
        ("s,symbols", "Symbols",            cxxopts::value<std::string>()->default_value("BTCUSDT"))
        ("t,threads", "Threads",            cxxopts::value<int>()->default_value("1"))
        ("r,duration","Duration",           cxxopts::value<int>()->default_value("60"))
        ("i,interval","Interval",           cxxopts::value<int>()->default_value("1"))
        ("h,help",    "Print usage")
    ;
    auto result = options.parse(argc, argv);
    if (result.count("help")){
      std::cout << options.help() << std::endl;
      exit(0);
    }
    DEBUG = result["debug"].as<bool>();
    URL=result["url"].as<std::string>();
    SYMBOL=result["symbols"].as<std::string>();
    DURATION=result["duration"].as<int>();
    INTERVAL=result["interval"].as<int>();
    int N=result["threads"].as<int>();

    threads_boost(N);
    return 0;
}

