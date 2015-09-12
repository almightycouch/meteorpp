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

#ifndef __meteorpp_ddp_collection_hpp__
#define __meteorpp_ddp_collection_hpp__

#include <boost/bimap.hpp>

#include "ddp.hpp"
#include "collection.hpp"

namespace meteorpp {
    class ddp_collection : public collection
    {
        public:
        typedef boost::signals2::signal<void()> ready_signal;

        template<typename ...Args>
        ddp_collection(std::shared_ptr<ddp> const& ddp, std::string const& name, Args&&... args) throw(ejdb_exception, websocketpp::exception)
            : collection(name), _name(name), _ddp(ddp)
        {
            init_ddp_collection(name, { std::forward<Args>(args)... });
        }

        template<typename ...Args>
        ddp_collection(std::shared_ptr<ddp> const& ddp, std::pair<std::string, std::string> const& name_pair, Args&&... args) throw(ejdb_exception, websocketpp::exception)
            : collection(name_pair.first), _name(name_pair.first), _ddp(ddp)
        {
            init_ddp_collection(name_pair.second, { std::forward<Args>(args)... });
        }

        virtual ~ddp_collection();

        virtual std::string insert(nlohmann::json::object_t const& document) throw(std::runtime_error);

        virtual int update(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error);

        virtual int upsert(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error);

        virtual int remove(nlohmann::json::object_t const& selector) throw(std::runtime_error);

        void on_ready(ready_signal::slot_type const& slot);

        private:
        void init_ddp_collection(std::string const& name, nlohmann::json::array_t const& params = nlohmann::json::array()) throw(websocketpp::exception);

        void commit_insert(std::string const& id, nlohmann::json::object_t const& fields);

        void commit_update(std::string const& id, nlohmann::json::object_t const& fields, std::vector<std::string> const& cleared);

        void commit_remove(std::string const& id);

        void on_initial_batch(std::string const& subscription);

        void on_document_added(std::string const& collection, std::string const& id, nlohmann::json::object_t const& fields);

        void on_document_changed(std::string const& collection, std::string const& id, nlohmann::json::object_t const& fields, std::vector<std::string> const& cleared);

        void on_document_removed(std::string const& collection, std::string const& id);

        private:
        std::string _name;
        std::shared_ptr<ddp> _ddp;
        std::string _subscription;
        ready_signal _ready_sig;
        boost::bimap<std::string, std::string> _idle;
        boost::signals2::connection _doc_insert_push;
        boost::signals2::connection _doc_update_push;
        boost::signals2::connection _doc_remove_push;
    };
}
#endif
