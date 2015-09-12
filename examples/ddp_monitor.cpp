#include <iostream>

#include <boost/asio.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <meteorpp/ddp.hpp>
#include <meteorpp/ddp_collection.hpp>
#include <meteorpp/live_query.hpp>

namespace boost {
    namespace po = program_options;
}

void print_live_query(std::shared_ptr<meteorpp::live_query> const& live_query)
{
    std::system("clear");
    std::cout << live_query->data().dump(4) << std::endl;
}

int main(int argc, char** argv)
{
    boost::po::options_description desc("options");
    desc.add_options()
        ("help,h",
            "display this help message and exit")
        ("version",
            "display version informations and exit")
        ("ws", boost::po::value<std::string>()->value_name("url"),
            "connect to the given websocket url\ndefault: ws://locahost:3000/websocket")
    ;

    boost::po::options_description hidden;
    hidden.add_options()
        ("name", boost::po::value<std::string>())
    ;

    boost::po::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    boost::po::positional_options_description pos;
    pos.add("name", 1);

    boost::po::variables_map cli;
    try {
        boost::po::store( boost::po::command_line_parser(argc, argv).options(cmdline_options).positional(pos).run(), cli);
    } catch(boost::po::error const& e) {
        std::cerr << "error: " << e.what() << std::endl;
    }

    boost::po::notify(cli);
    if(cli.count("version")) {
        std::cout << "ddp-monitor 0.1.1" << std::endl;
        return 0;
    } else if(cli.count("help") || !cli.count("name")) {
        std::cout << "usage: " << argv[0] << " <name> [arg1] [arg2] ... [argN] [options]" << std::endl;
        std::cout << std::endl;
        std::cout << desc << std::endl;
        return 0;
    }

    try {
        std::shared_ptr<meteorpp::ddp_collection> coll;
        std::shared_ptr<meteorpp::live_query> live_query;

        boost::asio::io_service io;

        auto url = cli.count("ws") ? cli["ws"].as<std::string>() : "ws://localhost:3000/websocket";
        auto ddp = std::make_shared<meteorpp::ddp>(io);
        ddp->connect(url, [&](std::string const& session) {
            coll = std::make_shared<meteorpp::ddp_collection>(ddp, cli["name"].as<std::string>());
            coll->on_ready([&]() {
                live_query = coll->track();
                live_query->on_changed(std::bind(print_live_query, live_query));
                print_live_query(live_query);
            });
        });

        io.run();
    } catch(std::exception const& e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    std::cout << "disconnected" << std::endl;

    return 0;
}
