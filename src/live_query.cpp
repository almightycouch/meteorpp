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

#include <ejdb/ejdb.h>
#include <ejdb/ejdb_private.h>

#include "../include/meteorpp/live_query.hpp"

namespace meteorpp {
    live_query::live_query(nlohmann::json::object_t const& selector, std::shared_ptr<collection> const& collection) throw(std::runtime_error)
        : _selector(collection::convert_to_bson(selector)), _coll(collection)
    {
        _coll->document_added.connect(std::bind(&live_query::document_added, this, std::placeholders::_1, std::placeholders::_2));
        _coll->document_pre_changed.connect(std::bind(&live_query::document_changed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        _coll->document_pre_removed.connect(std::bind(&live_query::document_removed, this, std::placeholders::_1, std::placeholders::_2));
        _results = _coll->find(selector);
    }

    live_query::~live_query()
    {
    }

    nlohmann::json const& live_query::data() const
    {
        return _results;
    }

    boost::signals2::connection live_query::on_updated(updated_signal::slot_type const& slot)
    {
        return _updated_sig.connect(slot);
    }

    boost::signals2::connection live_query::on_document_added(collection::document_added_signal::slot_type const& slot)
    {
        return _doc_added_sig.connect(slot);
    }

    boost::signals2::connection live_query::on_document_changed(collection::document_changed_signal::slot_type const& slot)
    {
        return _doc_changed_sig.connect(slot);
    }

    boost::signals2::connection live_query::on_document_removed(collection::document_removed_signal::slot_type const& slot)
    {
        return _doc_removed_sig.connect(slot);
    }

    void live_query::document_added(std::string const& id, nlohmann::json::object_t const& fields)
    {
        auto document = fields;
        document["_id"] = id;
        if(match(document)) {
            _results.push_back(document);
            _doc_added_sig(id, fields);
            _updated_sig();
        }
    }

    void live_query::document_changed(std::string const& id, nlohmann::json::object_t const& before, nlohmann::json::object_t const& after)
    {
        if(match(before)) {
            if(match(after)) {
                *std::find_if(_results.begin(), _results.end(),[id](nlohmann::json const& result) {
                    return result["_id"] == id;
                }) = after;
                auto const diff = collection::modified_fields(before, after);
                _doc_changed_sig(id, diff["fields"], diff["cleared"]);
            } else {
                _results.erase(std::remove_if(_results.begin(), _results.end(), [id](nlohmann::json const& result) {
                    return result["_id"] == id;
                }), _results.end());
                _doc_removed_sig(id);
            }
        } else if(match(after)) {
            _results.push_back(after);
            auto fields = after;
            fields.erase("_id");
            _doc_added_sig(id, fields);
        }
        _updated_sig();
    }

    void live_query::document_removed(std::string const& id, nlohmann::json::object_t const& document)
    {
        if(match(document)) {
            _results.erase(std::remove_if(_results.begin(), _results.end(), [id](nlohmann::json const& result) {
                return result["_id"] == id;
            }), _results.end());
            _doc_removed_sig(id);
            _updated_sig();
        }
    }

    bool live_query::match(nlohmann::json::object_t const& document)
    {
        return ejdbqrymatch(std::shared_ptr<EJQ>(ejdbcreatequery2(_coll->_db.get(), bson_data(_selector.get())), ejdbquerydel).get(), collection::convert_to_bson(document).get());
    }
}
