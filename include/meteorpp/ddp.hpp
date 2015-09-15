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

#ifndef __meteorpp_ddp_hpp__
#define __meteorpp_ddp_hpp__

#include <boost/signals2/signal.hpp>

#include <nlohmann/json.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

namespace meteorpp {
    class ddp
    {
        typedef websocketpp::client<websocketpp::config::asio> client;

        public:
        typedef boost::signals2::signal<void(std::string const& session)> connected_signal;
        typedef boost::signals2::signal<void(std::string const& id)> ready_signal;
        typedef boost::signals2::signal<void(std::string const& id, nlohmann::json const& result, nlohmann::json const& error)> method_result_signal;
        typedef boost::signals2::signal<void(std::string const& id)> method_updated_signal;
        typedef boost::signals2::signal<void(std::string const& collection, std::string const& id, nlohmann::json::object_t const& fields)> document_added_signal;
        typedef boost::signals2::signal<void(std::string const& collection, std::string const& id, nlohmann::json::object_t const& fields, std::vector<std::string> const& cleared)> document_changed_signal;
        typedef boost::signals2::signal<void(std::string const& collection, std::string const& id)> document_removed_signal;

        /* Constructs a ddp object.
         */
        ddp(boost::asio::io_service& io_service, std::string const& session = std::string());

        virtual ~ddp();

        /* Returns the current session id.
         */
        std::string session() const;

        /* Attempts to establish a WebSocket connection to a Meteor app.
         */
        void connect(std::string const& url = "ws://locahost:3000/websocket", connected_signal::slot_type const& slot = connected_signal::slot_function_type()) throw(websocketpp::exception);

        /* Invokes a server method passing any number of arguments.
         */
        std::string call_method(std::string const& name, nlohmann::json::array_t const& params = nlohmann::json::array(), method_result_signal::slot_type const& slot = method_result_signal::slot_function_type()) throw(websocketpp::exception);

        /* Subscribes to a record set.
         */
        std::string subscribe(std::string const& name, nlohmann::json::array_t const& params = nlohmann::json::array(), ready_signal::slot_type const& slot = ready_signal::slot_function_type()) throw(websocketpp::exception);

        /* Unsubscribes from a record set.
         */
        void unsubscribe(std::string const& id) throw(websocketpp::exception);

        boost::signals2::connection on_connected(connected_signal::slot_type const& slot);

        boost::signals2::connection on_ready(ready_signal::slot_type const& slot);

        boost::signals2::connection on_synchronized(method_updated_signal::slot_type const& slot);

        boost::signals2::connection on_document_added(document_added_signal::slot_type const& slot);

        boost::signals2::connection on_document_changed(document_changed_signal::slot_type const& slot);

        boost::signals2::connection on_document_removed(document_removed_signal::slot_type const& slot);

        private:
        std::string random_id(unsigned int length = 17) const;

        std::string random_method_id() const;

        void init_session();

        void on_message(client::message_ptr const& msg);

        private:
        client::connection_ptr _conn;
        client _client;
        std::string _session;
        connected_signal _connected_sig;
        ready_signal _ready_sig;
        method_result_signal _method_result_sig;
        method_updated_signal _method_updated_sig;
        document_added_signal _doc_added_sig;
        document_changed_signal _doc_changed_sig;
        document_removed_signal _doc_removed_sig;
    };
}

#endif
