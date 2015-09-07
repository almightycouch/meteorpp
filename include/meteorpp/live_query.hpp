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

#ifndef __meteorpp_live_query_hpp__
#define __meteorpp_live_query_hpp__

#include "collection.hpp"

namespace meteorpp {
    class live_query
    {
        public:
        typedef boost::signals2::signal<void()> updated_signal;

        live_query(nlohmann::json::object_t const& selector, std::shared_ptr<collection> const& collection) throw(std::runtime_error);

        virtual ~live_query();

        nlohmann::json const& data() const;

        boost::signals2::connection on_updated(updated_signal::slot_type const& slot);

        boost::signals2::connection on_document_added(collection::document_added_signal::slot_type const& slot);

        boost::signals2::connection on_document_changed(collection::document_changed_signal::slot_type const& slot);

        boost::signals2::connection on_document_removed(collection::document_removed_signal::slot_type const& slot);

        private:
        void document_added(std::string const& id, nlohmann::json::object_t const& fields);

        void document_changed(std::string const& id, nlohmann::json::object_t const& before, nlohmann::json::object_t const& after);

        void document_removed(std::string const& id, nlohmann::json::object_t const& document);

        bool match(nlohmann::json::object_t const& document);

        private:
        nlohmann::json _results;
        updated_signal _updated_sig;
        collection::document_added_signal _doc_added_sig;
        collection::document_changed_signal _doc_changed_sig;
        collection::document_removed_signal _doc_removed_sig;
        std::shared_ptr<bson> _selector;
        std::shared_ptr<collection> _coll;
    };
}

#endif
