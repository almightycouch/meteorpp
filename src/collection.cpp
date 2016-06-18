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

#include <mutex>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>

#include <ejdb/ejdb.h>
#include <ejdb/ejdb_private.h>

#include "../include/meteorpp/collection.hpp"
#include "../include/meteorpp/live_query.hpp"


std::weak_ptr<EJDB> db;

std::once_flag bson_oid_setup_flag;

namespace meteorpp {
    ejdb_exception::ejdb_exception(std::string const& error_message, int error_code)
        : std::runtime_error(error_message), _error_code(error_code)
    {
    }

    ejdb_exception::ejdb_exception(int error_code)
        : std::runtime_error(ejdberrmsg(error_code)), _error_code(error_code)
    {
    }

    ejdb_exception::~ejdb_exception()
    {

    }

    int ejdb_exception::error_code() const
    {
        return _error_code;
    }

    collection::collection(std::string const& name) throw(ejdb_exception)
    {
        if(name.empty()) {
            throw ejdb_exception(JBEINVALIDCOLNAME);
        }

        std::call_once(bson_oid_setup_flag, std::bind(bson_set_oid_inc, []() -> int {
            static int i = 0;
            return i++;
        }));
        if(!(_db = db.lock())) {
            _db = std::shared_ptr<EJDB>(ejdbnew(), ejdbdel);
            if(!ejdbopen(_db.get(), "meteorpp.db", JBOWRITER | JBOCREAT | JBOTRUNC)) {
                throw_last_ejdb_exception();
            }
            db = _db;
        }

        auto* ejdb_coll = ejdbcreatecoll(_db.get(), name.c_str(), nullptr);
        if(!ejdb_coll) {
            throw_last_ejdb_exception();
        }
        _coll.reset(ejdb_coll, ejdbsyncoll);
    }

    collection::~collection()
    {
    }

    std::shared_ptr<live_query> collection::track(nlohmann::json::object_t const& selector) throw(std::bad_weak_ptr)
    {
        return std::make_shared<live_query>(selector, shared_from_this());
    }

    int collection::count(nlohmann::json::object_t const& selector) throw(std::runtime_error)
    {
        return query(selector, nlohmann::json::object(), JBQRYCOUNT);
    }

    std::vector<nlohmann::json::object_t> collection::find(nlohmann::json::object_t const& selector) throw(std::runtime_error)
    {
        return query(selector);
    }

    nlohmann::json::object_t collection::find_one(nlohmann::json::object_t const& selector) throw(std::runtime_error)
    {
        return query(selector, nlohmann::json::object(), JBQRYFINDONE);
    }

    std::string collection::insert(nlohmann::json::object_t const& document) throw(std::runtime_error)
    {
        bson_oid_t oid;

        std::shared_ptr<bson> bson_doc = convert_to_bson(document);
        if(document.find("_id") != document.end()) {
            std::string const &id = document.find("_id")->second;
            if(!ejdbisvalidoidstr(id.c_str())) {
                throw ejdb_exception(JBEINVALIDBSONPK);
            }
            bson_oid_from_string(&oid, id.c_str());
            std::shared_ptr<bson> bson_oid(bson_create(), bson_del);
            bson_init(bson_oid.get());
            bson_append_oid(bson_oid.get(), "_id", &oid);
            bson_finish(bson_oid.get());
            std::shared_ptr<bson> bson_doc_with_oid(bson_create(), bson_del);
            bson_init(bson_doc_with_oid.get());
            bson_merge(bson_oid.get(), bson_doc.get(), true, bson_doc_with_oid.get());
            bson_finish(bson_doc_with_oid.get());
            bson_doc.swap(bson_doc_with_oid);
        }

        if(!ejdbsavebson(_coll.get(), bson_doc.get(), &oid)) {
            throw_last_ejdb_exception();
        }

        std::string id(24, '\0');
        bson_oid_to_string(&oid, &id[0]);

        document_added(id, document);
        return id;
    }

    int collection::update(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error)
    {
        return query(selector, modifier).size();
    }

    int collection::upsert(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error)
    {
        return query(selector, {{ "$upsert", modifier }}).size();
    }

    int collection::remove(nlohmann::json::object_t const& selector) throw(std::runtime_error)
    {
        return query(selector, {{ "$dropall", true }}).size();
    }

    nlohmann::json collection::query(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier, int flags) throw(ejdb_exception)
    {
        nlohmann::json q1 = selector;
        nlohmann::json q2 = modifier;

        bool with_modifier = !q2.empty();
        if(with_modifier) {
            std::swap(q1, q2);
        }

        auto* ejdb_query = ejdbcreatequery(_db.get(), convert_to_bson(q1).get(), with_modifier ? convert_to_bson(q2).get() : nullptr, (int)with_modifier, nullptr);
        if(!ejdb_query) {
            throw_last_ejdb_exception();
        }

        uint32_t count;
        std::shared_ptr<TCXSTR> log(tcxstrnew(), tcxstrdel);
        auto cursor = ejdbqryexecute(_coll.get(), ejdb_query, &count, flags, log.get());
        ejdbquerydel(ejdb_query);

        nlohmann::json updates;
        nlohmann::json upserts;
        nlohmann::json dropall;
        nlohmann::json const json_log = evaluate_log(std::string(log->ptr, log->size));
        if(json_log.find("updating_mode") != json_log.end() && json_log["updating_mode"]) {
            if(json_log.find("$update") != json_log.end()) {
                updates = json_log["$update"];
                if(updates.type() != nlohmann::json::value_t::array) {
                    updates = nlohmann::json::array_t({ updates });
                }
            }
            if(json_log.find("$upsert") != json_log.end()) {
                upserts = json_log["$upsert"];
                if(upserts.type() != nlohmann::json::value_t::array) {
                    upserts = nlohmann::json::array_t({ upserts });
                }
                for(nlohmann::json::object_t doc: upserts) {
                    std::string const id = doc["_id"];
                    doc.erase("_id");
                    document_added(id, doc);
                }
            }
            if(json_log.find("$dropall") != json_log.end()) {
                for(std::string const& id: json_log["$dropall"]) {
                    dropall.push_back(id);
                }
            }
        }

        nlohmann::json return_val;
        if(flags & JBQRYCOUNT) {
            return_val = count;
        } else {
            std::vector<nlohmann::json::object_t> results;
            results.reserve(count);
            for(int i = 0; i < count; ++i) {
                int data_size = 0;
                auto const& result = convert_to_json(std::shared_ptr<bson>(bson_create_from_buffer(static_cast<char const*>(ejdbqresultbsondata(cursor, i, &data_size)), data_size), bson_destroy));
                if(i < updates.size()) {
                    auto const& update = updates[i];
                    if(result != update) {
                        auto const diff = modified_fields(result, update);
                        document_pre_changed(result["_id"], result, update);
                        document_changed(result["_id"], diff["fields"], diff["cleared"]);
                    }
                }
                if(i < dropall.size()) {
                    std::string const& id = dropall[i];
                    if(result["_id"] == id) {
                        document_pre_removed(id, result);
                        document_removed(id);
                    }

                }
                results.push_back(result);
            }
            ejdbqresultdispose(cursor);
            return_val = results;
        }

        if(flags & JBQRYFINDONE) {
            return_val = return_val.empty() ? nlohmann::json::object() : return_val[0];
        }

        return return_val;
    }

    nlohmann::json collection::evaluate_log(std::string const& log)
    {
        nlohmann::json json_log;
        std::istringstream log_stream(log);
        for(std::string line; std::getline(log_stream, line); ) {
            auto const delimiter = line.find_first_of(':');
            if(delimiter != std::string::npos) {
                std::string key = line.substr(0, delimiter);
                std::string val = line.substr(delimiter + 2);
                std::replace(key.begin(), key.end(), ' ', '_');
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                nlohmann::json json_val;
                try {
                    json_val = boost::lexical_cast<long>(val);
                } catch(boost::bad_lexical_cast const& e) {
                    if(val == "YES") {
                        json_val = true;
                    } else if(val == "NO") {
                        json_val = false;
                    } else if(val == "'NONE'") {
                        json_val = nullptr;
                    } else {
                        size_t const bson_hex_delimiter = val.find_first_of('/');
                        if(bson_hex_delimiter != std::string::npos) {
                            std::string const bson_hex_string = val.substr(0, bson_hex_delimiter);
                            std::string bson_decoded_data;
                            boost::algorithm::unhex(bson_hex_string.begin(), bson_hex_string.end(), back_inserter(bson_decoded_data));
                            json_val = convert_to_json(std::shared_ptr<bson>(bson_create_from_buffer(bson_decoded_data.data(), bson_decoded_data.size()), bson_del));
                        } else {
                            json_val = val;
                        }
                    }
                }

                if(json_log.find(key) != json_log.end()) {
                    if(json_log[key].type() != nlohmann::json::value_t::array) {
                        json_log[key] = nlohmann::json::array_t({ json_log[key] });
                    }
                    json_log[key].push_back(json_val);
                } else {
                    json_log[key] = json_val;

                }
            }
        }
        return json_log;
    }

    void collection::throw_last_ejdb_exception() throw(ejdb_exception)
    {
        throw ejdb_exception(ejdbecode(_db.get()));
    }

    nlohmann::json collection::modified_fields(nlohmann::json::object_t const& a, nlohmann::json::object_t const& b)
    {
        nlohmann::json::object_t diff;
        std::set_difference(b.begin(), b.end(), a.begin(), a.end(), std::inserter(diff, diff.begin()));

        std::vector<std::string> cleared_fields;
        for(auto const& field: a) {
            if(b.find(field.first) == b.end()) {
                cleared_fields.push_back(field.first);
            }
        }
        return {{ "fields", diff }, { "cleared", cleared_fields }};
    }

    nlohmann::json collection::convert_to_json(std::shared_ptr<bson> const& value)
    {
        char* buffer;
        int size = 0;
        bson2json(bson_data(value.get()), &buffer, &size);
        return nlohmann::json::parse(std::string(buffer, size));
    }

    std::shared_ptr<bson> collection::convert_to_bson(nlohmann::json const& value)
    {
        return std::shared_ptr<bson>(json2bson(value.dump().c_str()), bson_del);
    }
}
