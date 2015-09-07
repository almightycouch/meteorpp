/*
 * Copyright (c) 2015, Mario Flach. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "../include/meteorpp/ddp.hpp"

namespace meteorpp {
    ddp::ddp(boost::asio::io_service& io_service, std::string const& session)
        : _session(session)
    {
        _client.init_asio(&io_service);
        _client.set_open_handler(std::bind(&ddp::init_session, this));
        _client.set_message_handler(std::bind(&ddp::on_message, this, std::placeholders::_2));
        _client.clear_access_channels(websocketpp::log::alevel::all);
        _client.clear_error_channels(websocketpp::log::alevel::all);
    }

    ddp::~ddp()
    {
    }

    std::string ddp::session() const
    {
        return _session;
    }

    void ddp::connect(std::string const& url, connected_signal::slot_type const& slot) throw(websocketpp::exception)
    {
        websocketpp::lib::error_code error_code;
        _conn = _client.get_connection(url, error_code);
        if(error_code) {
            throw websocketpp::exception("Connection error", error_code);
        }

        _connected_sig.connect_extended([=](boost::signals2::connection const& conn, std::string const& session) {
            slot(session);
            conn.disconnect();
        });
        _client.connect(_conn);
    }

    std::string ddp::call_method(std::string const& name, nlohmann::json::array_t const& params, method_result_signal::slot_type const& slot) throw(websocketpp::exception)
    {
        auto i = random_method_id();
        if(slot.slot_function()) {
            _method_result_sig.connect_extended([=](boost::signals2::connection const& conn, std::string const& id, nlohmann::json const& result, nlohmann::json const& error) {
                if(id == i) {
                    slot(id, result, error);
                    conn.disconnect();
                }
            });
        }

        nlohmann::json payload;
        payload["msg"] = "method";
        payload["method"] = name;
        payload["id"] = i;
        payload["params"] = params;
        _conn->send(payload.dump(), websocketpp::frame::opcode::text);

        return i;
    }

    std::string ddp::subscribe(std::string const& name, nlohmann::json::array_t const& params, ready_signal::slot_type const& slot) throw(websocketpp::exception)
    {
        auto const i = random_id();
        if(slot.slot_function()) {
            _ready_sig.connect_extended([=](boost::signals2::connection const& conn, std::string const& id) {
                if(id == i) {
                    slot(i);
                    conn.disconnect();
                }
            });
        }

        nlohmann::json payload;
        payload["msg"] = "sub";
        payload["name"] = name;
        payload["id"] = i;
        payload["params"] = params;
        _conn->send(payload.dump(), websocketpp::frame::opcode::text);

        return i;
    }

    void ddp::unsubscribe(std::string const& id) throw(websocketpp::exception)
    {
        nlohmann::json payload;
        payload["msg"] = "unsub";
        payload["id"] = id;
        _conn->send(payload.dump(), websocketpp::frame::opcode::text);
    }

    boost::signals2::connection ddp::on_connected(connected_signal::slot_type const& slot)
    {
        return _connected_sig.connect(slot);
    }

    boost::signals2::connection ddp::on_ready(ready_signal::slot_type const& slot)
    {
        return _ready_sig.connect(slot);
    }

    boost::signals2::connection ddp::on_synchronized(method_updated_signal::slot_type const& slot)
    {
        return _method_updated_sig.connect(slot);
    }

    boost::signals2::connection ddp::on_document_added(document_added_signal::slot_type const& slot)
    {
        return _doc_added_sig.connect(slot);
    }

    boost::signals2::connection ddp::on_document_changed(document_changed_signal::slot_type const& slot)
    {
        return _doc_changed_sig.connect(slot);
    }

    boost::signals2::connection ddp::on_document_removed(document_removed_signal::slot_type const& slot)
    {
        return _doc_removed_sig.connect(slot);
    }

    void ddp::init_session()
    {
        nlohmann::json payload;
        payload["msg"] = "connect";
        if(!_session.empty()) {
            payload["session"] = _session;
        }
        payload["version"] = "1";
        payload["support"] = { "1" };
        _conn->send(payload.dump(), websocketpp::frame::opcode::text);
    }

    std::string ddp::random_id(unsigned int length) const
    {
        std::string id;
        std::string const chars(
           "abcdefghijklmnopqrstuvwxyz"
           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           "1234567890"
        );
        boost::random::random_device random;
        boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
        for(unsigned int i = 0; i < length; ++i) {
            id += chars[index_dist(random)];
        }
        return id;
    }

    std::string ddp::random_method_id() const
    {
        static int i = 0;
        return std::to_string(i++);
    }

    void ddp::on_message(client::message_ptr const& msg)
    {
        auto const payload = nlohmann::json::parse(msg->get_payload());
        auto const message = payload["msg"];
        if(message.is_null()) {
            // ignore
        } else if(message == "connected") {
            _session = payload["session"].get<std::string>();
            _connected_sig(_session);
        } else if(message == "failed") {
            // throw error
        } else if(message == "ping") {
            nlohmann::json response;
            response["msg"] = "pong";
            response["id"] = payload["id"];
            _conn->send(response.dump(), websocketpp::frame::opcode::text);
        } else if(message == "error") {
            // throw error
        } else if(message == "nosub") {
            // throw error
        } else if(message == "added") {
            nlohmann::json const& fields = payload["fields"];
            _doc_added_sig(payload["collection"], payload["id"], !fields.is_null() ? fields : nlohmann::json::object());
        } else if(message == "changed") {
            nlohmann::json const& fields = payload["fields"];
            nlohmann::json const& cleared = payload["cleared"];
            _doc_changed_sig(payload["collection"], payload["id"], !fields.is_null() ? fields : nlohmann::json::object(), !cleared.is_null() ? cleared : nlohmann::json::array());
        } else if(message == "removed") {
            _doc_removed_sig(payload["collection"], payload["id"]);
        } else if(message == "ready") {
            for(std::string const& id: payload["subs"]) {
                _ready_sig(id);
            }
        } else if(message == "updated") {
            for(std::string const& id: payload["methods"]) {
                _method_updated_sig(id);
            }
        } else if(message == "result") {
            _method_result_sig(payload["id"], payload["result"], payload["error"]);
        }
    }
}
