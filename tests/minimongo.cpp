#define BOOST_TEST_MODULE minimongo

#include <boost/test/included/unit_test.hpp>

#include <meteorpp/collection.hpp>

struct fixture {
    fixture() : coll(std::make_shared<meteorpp::collection>("test")) {}
    std::shared_ptr<meteorpp::collection> coll;
};

BOOST_AUTO_TEST_CASE(create_collection)
{
    BOOST_REQUIRE_NO_THROW(std::make_shared<meteorpp::collection>("test"));
}

BOOST_AUTO_TEST_CASE(create_invalid_collection)
{
    BOOST_CHECK_THROW(std::make_shared<meteorpp::collection>(""), meteorpp::ejdb_exception);
}

BOOST_FIXTURE_TEST_CASE(insert_with_oid, fixture)
{
    std::string const oid = "d776bb695e5447997999b1fd";
    BOOST_CHECK_EQUAL(coll->insert({{ "_id", oid }, { "foo", "bar" }}), oid);
}

BOOST_FIXTURE_TEST_CASE(insert_with_invalid_oid, fixture)
{
    std::string const oid = "0xe5505";
    BOOST_CHECK_THROW(coll->insert({{ "_id", oid }, { "foo", "bar" }}), meteorpp::ejdb_exception);
}

BOOST_FIXTURE_TEST_CASE(insert_find_one, fixture)
{
    nlohmann::json::object_t doc = {{ "foo", "bar" }};
    doc.insert(std::make_pair("_id", coll->insert(doc)));
    BOOST_CHECK_EQUAL(coll->find_one(), doc);
}

BOOST_FIXTURE_TEST_CASE(insert_find_multi, fixture)
{
    nlohmann::json::object_t doc1 = {{ "foo", "bar" }};
    doc1.insert(std::make_pair("_id", coll->insert(doc1)));

    nlohmann::json::object_t doc2 = {{ "foo", "baz" }};
    doc2.insert(std::make_pair("_id", coll->insert(doc2)));

    auto const excepted = { doc1, doc2 };
    BOOST_CHECK_EQUAL(coll->find(), excepted);
}

BOOST_FIXTURE_TEST_CASE(insert_update_one, fixture)
{
    nlohmann::json::object_t doc = {{ "foo", "bar" }};
    doc.insert(std::make_pair("_id", coll->insert(doc)));

    auto const it = coll->update(doc, {{ "$set", {{ "foo", "baz" }}}});
    BOOST_CHECK_EQUAL(it, 1);

    doc["foo"] = "baz";
    BOOST_CHECK_EQUAL(coll->find_one(), doc);
}

BOOST_FIXTURE_TEST_CASE(insert_update_multi, fixture)
{
    nlohmann::json::object_t doc1 = {{ "foo", "bar" }};
    doc1.insert(std::make_pair("_id", coll->insert(doc1)));

    nlohmann::json::object_t doc2 = {{ "foo", "baz" }};
    doc2.insert(std::make_pair("_id", coll->insert(doc2)));

    auto const it = coll->update({}, {{ "$set", {{ "bar", "foo" }}}});
    BOOST_CHECK_EQUAL(it, 2);

    doc1["bar"] = "foo";
    doc2["bar"] = "foo";

    auto const excepted = { doc1, doc2 };
    BOOST_CHECK_EQUAL(coll->find(), excepted);
}

BOOST_FIXTURE_TEST_CASE(insert_multi_update_one, fixture)
{
    nlohmann::json::object_t doc1 = {{ "foo", "bar" }};
    doc1.insert(std::make_pair("_id", coll->insert(doc1)));

    nlohmann::json::object_t doc2 = {{ "foo", "baz" }};
    doc2.insert(std::make_pair("_id", coll->insert(doc2)));

    auto const it = coll->update(doc1, {{ "$set", {{ "bar", "foo" }}}});
    BOOST_CHECK_EQUAL(it, 1);

    doc1["bar"] = "foo";

    auto const result_vect = coll->find();
    std::set<nlohmann::json::object_t> const result_set(result_vect.begin(), result_vect.end());
    std::set<nlohmann::json::object_t> const excepted = { doc1, doc2 };
    BOOST_CHECK_EQUAL(result_set, excepted);
}

BOOST_FIXTURE_TEST_CASE(insert_remove_one, fixture)
{
    auto const id = coll->insert({{ "foo", "bar" }});
    auto const it = coll->remove({{ "_id", id }});

    BOOST_CHECK_EQUAL(it, 1);
    BOOST_CHECK(coll->find_one().empty());
}

BOOST_FIXTURE_TEST_CASE(insert_remove_multi, fixture)
{
    coll->insert({{ "foo", "bar" }});
    coll->insert({{ "foo", "baz" }});
    auto const it = coll->remove({});

    BOOST_CHECK_EQUAL(it, 2);
    BOOST_CHECK(coll->find_one().empty());
}

BOOST_FIXTURE_TEST_CASE(insert_multi_remove_one, fixture)
{
    nlohmann::json::object_t doc1 = {{ "foo", "bar" }};
    doc1.insert(std::make_pair("_id", coll->insert(doc1)));

    nlohmann::json::object_t doc2 = {{ "foo", "baz" }};
    doc2.insert(std::make_pair("_id", coll->insert(doc2)));

    auto const it = coll->remove(doc1);

    BOOST_CHECK_EQUAL(it, 1);
    BOOST_CHECK_EQUAL(coll->find_one(), doc2);
}
