//
//  Socket.cpp
//  websocketppDemo
//
//  Created by Anton Katekov on 2022/01/13.
//

#include "Socket.hpp"
#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

typedef websocketpp::client<websocketpp::config::asio_client> client;

enum connection_metadata_status {
    connection_metadata_status_connecting,
    connection_metadata_status_open,
    connection_metadata_status_failed,
    connection_metadata_status_closed
};

class connection_metadata {
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;
    

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status(connection_metadata_status::connection_metadata_status_connecting)
      , m_uri(uri)
      , m_server("N/A")
    {}

    void on_open(client * c, websocketpp::connection_hdl hdl) {
        m_status = connection_metadata_status::connection_metadata_status_open;

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        m_status = connection_metadata_status::connection_metadata_status_failed;

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }
    
    void on_close(client * c, websocketpp::connection_hdl hdl) {
        m_status = connection_metadata_status::connection_metadata_status_closed;
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        std::stringstream s;
        s << "close code: " << con->get_remote_close_code() << " (" 
          << websocketpp::close::status::get_string(con->get_remote_close_code()) 
          << "), close reason: " << con->get_remote_close_reason();
        m_error_reason = s.str();
    }
    
    void on_message(client * c, websocketpp::connection_hdl hdl, websocketpp::config::core_client::message_type::ptr msg) {
        
    }

    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }
    
    int get_id() const {
        return m_id;
    }
    
    connection_metadata_status get_status() const {
        return m_status;
    }

    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    connection_metadata_status m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
};

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason);

    return out;
}

class websocket_endpoint {
    client::connection_ptr con;
    
public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }

    ~websocket_endpoint() {
        m_endpoint.stop_perpetual();
        
        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
            if (it->second->get_status() != connection_metadata_status::connection_metadata_status_open) {
                // Only close open connections
                continue;
            }
            
            std::cout << "> Closing connection " << it->second->get_id() << std::endl;
            
            websocketpp::lib::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec) {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                          << ec.message() << std::endl;
            }
        }
        
        m_thread->join();
    }

    int connect(std::string const & uri) {
        websocketpp::lib::error_code ec;

        con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            con = client::connection_ptr();
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }

        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri));
        m_connection_list[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_close_handler(websocketpp::lib::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_message_handler(websocketpp::lib::bind(
             &connection_metadata::on_message,
             metadata_ptr,
             &m_endpoint,
             websocketpp::lib::placeholders::_1,
             websocketpp::lib::placeholders::_2
         ));

        m_endpoint.connect(con);

        return new_id;
    }

    void close(int id, websocketpp::close::status::value code, std::string reason) {
        websocketpp::lib::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }
    
    void send(const std::string &message) {
        auto ec = con->send(message);
        if (ec) {
            std::cout << "> Error sending: " << ec.message() << std::endl;
        }
    }

    connection_metadata::ptr get_metadata(int id) const {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            return connection_metadata::ptr();
        } else {
            return metadata_it->second;
        }
    }
private:
    typedef std::map<int,connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};


int Socket::Send() {
    websocket_endpoint endpoint;
    int id = endpoint.connect("");
    if (id != -1) {
        std::cout << "> Created connection with id " << id << std::endl;
    }

    auto metadata = endpoint.get_metadata(id);
    while (metadata->get_status() == connection_metadata_status::connection_metadata_status_connecting) {
        std::this_thread::sleep_for (std::chrono::milliseconds(10));
    }
    
    if (metadata) {
        std::cout << *metadata << std::endl;
    } 
    
    endpoint.send("hello");
    
    if (metadata->get_status() != connection_metadata_status::connection_metadata_status_open) {
        return 1;
    }
    {
        int close_code = websocketpp::close::status::normal;
        std::string reason = "bye";
        
        endpoint.close(id, close_code, reason);
    }
    while (metadata->get_status() != connection_metadata_status::connection_metadata_status_closed) {
        std::this_thread::sleep_for (std::chrono::milliseconds(10));
    }
    
    return 0;
}

