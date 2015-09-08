Meteor++
========

[![Build Status](https://travis-ci.org/almightycouch/meteorpp.svg?branch=master)](https://travis-ci.org/almightycouch/meteorpp)
[![Coverage Status](https://coveralls.io/repos/almightycouch/meteorpp/badge.svg?branch=master&service=github)](https://coveralls.io/github/almightycouch/meteorpp)
[![Github Releases](https://img.shields.io/github/release/almightycouch/meteorpp.svg)](https://github.com/almightycouch/meteorpp/releases)
[![Documentation Status](https://img.shields.io/badge/docs-doxygen-blue.svg)](http://almightycouch.github.io/meteorpp)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/almightycouch/meteorpp/master/LICENSE)
[![Github Issues](https://img.shields.io/github/issues/almightycouch/meteorpp.svg)](http://github.com/almightycouch/meteorpp/issues)

Meteor DDP & Minimongo implementation in C++.

With this library you can:

* communicate with Meteor applications over Websockets
* subscribe to real-time feeds, track changes and observe specific queries
* call server-side methods, query and modify collections
* keep your data mirrored and simulate server operations (latency compensation)


Quick Start
------------

Install Meteor++:

    git clone git://github.com/almightycouch/meteorpp.git
    cmake .
    make
    make install

Run the examples:

    meteorpp localhost:3000 test

Intregrate the library to your project and enjoy the power of Meteor with the speed of C++.


Documentation
-------------

One of the [seven principles](http://docs.meteor.com/#/full/sevenprinciples) of Meteor is _"simplicity equals productivity"_.

No esoteric concepts, clear documentation, well-established coding conventions, a set of simple tools designed to integrate seamlessly together.

Meteor++ was built from ground up with the very same philosophy, keep things simple. Both APIs share many similar patterns.
If you are familiar to Meteor, you will feel right at home.

#### Integration
The library is written in modern C++. It depends on following third-party libraries:

* the delicious [nlohmann][] JSON library
* [Boost libraries][Boost], essentially [Boost.Asio][] and [Boost.Signals2][]
* [EJDB][] <sup id="a1">[patch](#f1)</sup>, a MongoDB engine written in C
* Zaphoyd's websocket implementation, [WebSocket++][]

Most of these libraries are header-only and do not require any specific installation. You must install `boost_random`, `boost_system` and `ejdb` to successfully build Meteor++.

> In order to support features such as latency compensation and atomic operations, some files in the [EJDB][] library must be patched.
> You can get the patch to apply</em> [here][patch]. <sup id="f1">[↩](#a1)</sup>

[patch]: https://github.com/Softmotions/ejdb/compare/master...almightycouch:meteorpp.patch

Though it's 2015 already, the support for C++11 is still a bit sparse.
Do not forget to set the necessary switches to enable C++11 (e.g., `-std=c++11` for GCC and Clang).

[EJDB]: http://ejdb.org/
[Boost]: http://www.boost.org/doc/libs/
[Boost.Asio]: http://www.boost.org/doc/libs/1_59_0/doc/html/asio.html
[Boost.Signals2]: http://www.boost.org/doc/libs/1_59_0/doc/html/signals2.html
[nlohmann]: https://github.com/nlohmann/json
[WebSocket++]: http://www.zaphoyd.com/websocketpp


Examples
--------

Here are some examples to give you an idea how to use the library.

```c++
#include <boost/asio.hpp>

#include <meteorpp/ddp.hpp>
#include <meteorpp/ddp_collection.hpp>
#include <meteorpp/live_query.hpp>

void print_live_query(std::shared_ptr<meteorpp::live_query> const& live_query)
{
    std::cout << live_query->data().dump(4) << std::endl;
}

int main(int argc, char** argv)
{
    boost::asio::io_service io;

    std::shared_ptr<meteorpp::ddp_collection> coll;
    std::shared_ptr<meteorpp::live_query> live_query;

    auto ddp = std::make_shared<meteorpp::ddp>(io);
    ddp->connect("ws://localhost:3000/websocket", [&](std::string const& id) {
        coll = std::make_shared<meteorpp::ddp_collection>(ddp, "test");
        coll->on_ready([&]() {
            live_query = coll->track({{ "foo", "bar" }});
            live_query->on_updated(std::bind(print_live_query, live_query));
            print_live_query(live_query);
        });
    });

    io.run();
    return 0;
}
```


License & Warranty
------------------

    Copyright (c) 2015 Mario Flach under the MIT License (MIT)

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

<p align="center">
    <img src="http://opensource.org/trademarks/opensource/OSI-Approved-License-200x276.png" width="100" height="138" />
</p>
