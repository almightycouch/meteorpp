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

#include <boost/signals2/shared_connection_block.hpp>

#include "../include/meteorpp/ddp_collection.hpp"

namespace meteorpp {
    ddp_collection::~ddp_collection()
    {
        _ddp->unsubscribe(_subscription);
    }

    std::string ddp_collection::insert(nlohmann::json::object_t const& document) throw(std::runtime_error)
    {
        boost::signals2::scoped_connection conn;
        if(!_doc_insert_push.connected()) {
            throw std::runtime_error("couldn't execute insert command, database not ready");
        } else if(_doc_insert_push.blocked()) {
            conn = document_added.connect(std::bind(&ddp_collection::commit_insert, this, std::placeholders::_1, std::placeholders::_2));
        }
        return collection::insert(document);
    }

    int ddp_collection::update(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error)
    {
        boost::signals2::scoped_connection conn;
        if(!_doc_update_push.connected()) {
            throw std::runtime_error("couldn't execute update command, database not ready");
        } else if(_doc_update_push.blocked()) {
            conn = document_changed.connect(std::bind(&ddp_collection::commit_update, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }
        return collection::update(selector, modifier);
    }

    int ddp_collection::upsert(nlohmann::json::object_t const& selector, nlohmann::json::object_t const& modifier) throw(std::runtime_error)
    {
        boost::signals2::scoped_connection conn1, conn2;
        if(!_doc_insert_push.connected() || !_doc_update_push.connected()) {
            throw std::runtime_error("couldn't execute upsert command, database not ready");
        } else if(_doc_insert_push.blocked() || _doc_update_push.blocked()) {
            conn1 = document_added.connect(std::bind(&ddp_collection::commit_insert, this, std::placeholders::_1, std::placeholders::_2));
            conn1 = document_added.connect(std::bind(&ddp_collection::commit_insert, this, std::placeholders::_1, std::placeholders::_2));
        }
        return collection::upsert(selector, modifier);
    }

    int ddp_collection::remove(nlohmann::json::object_t const& selector) throw(std::runtime_error)
    {
        boost::signals2::scoped_connection conn;
        if(!_doc_remove_push.connected()) {
            throw std::runtime_error("couldn't execute remove command, database not ready");
        } else if(_doc_insert_push.blocked() || _doc_update_push.blocked() || _doc_remove_push.blocked()) {
            conn = document_removed.connect(std::bind(&ddp_collection::commit_remove, this, std::placeholders::_1));
        }
        return collection::remove(selector);
    }

    void ddp_collection::on_ready(ready_signal::slot_type const& slot)
    {
        _ready_sig.connect_extended([=](boost::signals2::connection const& conn) {
            slot();
            conn.disconnect();
        });
    }

    void ddp_collection::init_ddp_collection(std::string const& name, nlohmann::json::array_t const& params) throw(websocketpp::exception)
    {
        _ddp->on_document_added(std::bind(&ddp_collection::on_document_added, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        _ddp->on_document_changed(std::bind(&ddp_collection::on_document_changed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _ddp->on_document_removed(std::bind(&ddp_collection::on_document_removed, this, std::placeholders::_1, std::placeholders::_2));
        _ddp->on_synchronized([&](std::string const& method_id) {
            _idle.left.erase(method_id);
        });
        _subscription = _ddp->subscribe(name, params, std::bind(&ddp_collection::on_initial_batch, this, std::placeholders::_1));
    }

    void ddp_collection::commit_insert(std::string const& id, nlohmann::json::object_t const& fields)
    {
        nlohmann::json doc_with_id = fields;
        doc_with_id["_id"] = {{ "$type", "oid"}, { "$value", id }};

        auto method_id = _ddp->call_method('/' + _name + "/insert", { doc_with_id }, [=](std::string const& id, nlohmann::json const& result, nlohmann::json const& error) {
            if(!error.empty()) {
                std::cerr << error << std::endl;
            }
        });
        _idle.left.insert(std::make_pair(method_id, id));
    }

    void ddp_collection::commit_update(std::string const& id, nlohmann::json::object_t const& fields, std::vector<std::string> const& cleared)
    {
        nlohmann::json selector;
        selector["_id"] = {{ "$type", "oid"}, { "$value", id }};

        nlohmann::json modifier;
        if(!fields.empty()) {
            modifier["$set"] = fields;
        }
        if(!cleared.empty()) {
            nlohmann::json::object_t unset_fields;
            for(auto const& field: cleared) {
                unset_fields[field] = true;
            }
            modifier["$unset"] = unset_fields;
        }
        auto method_id = _ddp->call_method('/' + _name + "/update", { selector, modifier}, [=](std::string const& id, nlohmann::json const& result, nlohmann::json const& error) {
            if(!error.empty()) {
                std::cerr << error << std::endl;
            }
        });
        _idle.left.insert(std::make_pair(method_id, id));
    }

    void ddp_collection::commit_remove(std::string const& id)
    {
        nlohmann::json selector;
        selector["_id"] = {{ "$type", "oid"}, { "$value", id }};
        auto method_id = _ddp->call_method('/' + _name + "/remove", { selector }, [=](std::string const& id, nlohmann::json const& result, nlohmann::json const& error) {
            if(!error.empty()) {
                std::cerr << error << std::endl;
            }
        });
        _idle.left.insert(std::make_pair(method_id, id));
    }

    void ddp_collection::on_initial_batch(std::string const& subscription)
    {
        _doc_insert_push = document_added.connect(std::bind(&ddp_collection::commit_insert, this, std::placeholders::_1, std::placeholders::_2));
        _doc_update_push = document_changed.connect(std::bind(&ddp_collection::commit_update, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        _doc_remove_push = document_removed.connect(std::bind(&ddp_collection::commit_remove, this, std::placeholders::_1));
        _ready_sig();
    }

    void ddp_collection::on_document_added(std::string const& collection, std::string const& id, nlohmann::json::object_t const& fields)
    {
        if(collection == _name) {
            boost::signals2::shared_connection_block block(_doc_insert_push);
            if(_idle.right.find(id) == _idle.right.end()) {
                nlohmann::json document = fields;
                document["_id"] = id;
                collection::insert(document);
            }
        }
    }

    void ddp_collection::on_document_changed(std::string const& collection, std::string const& id, nlohmann::json::object_t const& fields, std::vector<std::string> const& cleared)
    {
        if(collection == _name) {
            boost::signals2::shared_connection_block block(_doc_update_push);
            if(_idle.right.find(id) == _idle.right.end()) {
                nlohmann::json modifier;
                if(!fields.empty()) {
                    modifier["$set"] = fields;
                }
                if(!cleared.empty()) {
                    nlohmann::json::object_t unset_fields;
                    for(auto const& cleared_field: cleared) {
                        unset_fields[cleared_field] = true;
                    }
                    modifier["$unset"] = unset_fields;
                }
                collection::update({{ "_id", id }}, modifier);
            }
        }
    }

    void ddp_collection::on_document_removed(std::string const& collection, std::string const& id)
    {
        if(collection == _name) {
            boost::signals2::shared_connection_block block(_doc_remove_push);
            if(_idle.right.find(id) == _idle.right.end()) {
                collection::remove({{ "_id", id }});
            }
        }
    }
}
