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

#ifndef __meteorpp_collection_hpp__
#define __meteorpp_collection_hpp__

#include <boost/signals2/signal.hpp>

#include <ejdb/bson.h>
#include <nlohmann/json.hpp>

struct EJDB;
struct EJCOLL;

namespace meteorpp {
    class live_query;

    class collection_base
    {
        public:
        typedef boost::signals2::signal<void(std::string const& id, nlohmann::json::object_t const& fields)> document_added_signal;
        typedef boost::signals2::signal<void(std::string const& id, nlohmann::json::object_t const& fields, std::vector<std::string> const& cleared)> document_changed_signal;
        typedef boost::signals2::signal<void(std::string const& id)> document_removed_signal;

        virtual std::vector<nlohmann::json::object_t> find(nlohmann::json::object_t const& selector = {}) throw(std::runtime_error) = 0;

        virtual nlohmann::json::object_t find_one(nlohmann::json::object_t const& selector = {}) throw(std::runtime_error) = 0;

        virtual std::string insert(nlohmann::json::object_t const& document) throw(std::runtime_error) = 0;

        virtual int update(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error) = 0;

        virtual int upsert(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error) = 0;

        virtual int remove(nlohmann::json::object_t const& selector) throw(std::runtime_error) = 0;

        protected:
        document_added_signal document_added;
        document_changed_signal document_changed;
        document_removed_signal document_removed;
    };

    class ejdb_exception : public std::runtime_error
    {
        public:
        ejdb_exception(std::string const& error_message, int error_code);

        ejdb_exception(int error_code);

        virtual ~ejdb_exception();

        int error_code() const;

        private:
        int _error_code;
    };

    class collection : public collection_base, public std::enable_shared_from_this<collection>
    {
        typedef boost::signals2::signal<void(std::string const& id, nlohmann::json::object_t const& before, nlohmann::json::object_t const& after)> document_pre_changed_signal;
        typedef boost::signals2::signal<void(std::string const& id, nlohmann::json::object_t const& document)> document_pre_removed_signal;

        friend class live_query;

        public:
        collection(std::string const& name) throw(ejdb_exception);

        virtual ~collection();

        std::shared_ptr<live_query> track(nlohmann::json::object_t const& selector = {}) throw(std::bad_weak_ptr);

        std::vector<nlohmann::json::object_t> find(nlohmann::json::object_t const& selector = {}) throw(std::runtime_error);

        nlohmann::json::object_t find_one(nlohmann::json::object_t const& selector = {}) throw(std::runtime_error);

        virtual std::string insert(nlohmann::json::object_t const& document) throw(std::runtime_error);

        virtual int update(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error);

        virtual int upsert(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error);

        virtual int remove(nlohmann::json::object_t const& selector) throw(std::runtime_error);

        protected:
        nlohmann::json query(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier = {}, int flags = 0) throw(ejdb_exception);

        nlohmann::json evaluate_log(std::string const& log);

        void throw_last_ejdb_exception() throw(ejdb_exception);

        protected:
        static nlohmann::json modified_fields(nlohmann::json::object_t const& a, nlohmann::json::object_t const& b);

        static nlohmann::json convert_to_json(std::shared_ptr<bson> const& value);

        static std::shared_ptr<bson> convert_to_bson(nlohmann::json const& value);

        private:
        document_pre_changed_signal document_pre_changed;
        document_pre_removed_signal document_pre_removed;
        std::shared_ptr<EJDB> _db;
        std::shared_ptr<EJCOLL> _coll;
    };
}

#endif
