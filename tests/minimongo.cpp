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

BOOST_FIXTURE_TEST_CASE(insert_and_find_first, fixture)
{
    nlohmann::json document = {{ "foo", "bar" }};
    auto const id = coll->insert(document);
    document["_id"] = id;
    BOOST_CHECK_EQUAL(coll->find_one(), document);
}
