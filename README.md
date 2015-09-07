Meteor++
========

Meteor DDP & Minimongo implementation in C++.


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
