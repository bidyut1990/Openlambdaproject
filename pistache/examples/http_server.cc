/* http_server.cc
   Mathieu Stefani, 07 février 2016

   Example of an http server
*/

#include <pistache/net.h>
#include <pistache/http.h>
#include <pistache/peer.h>
#include <pistache/http_headers.h>
#include <pistache/cookie.h>
#include <pistache/endpoint.h>
#include <curl/curl.h>
using namespace std;
using namespace Pistache;

int getFilenameAndIp(string sp, string  &filename, string &ip, string &cd)
{

        string delimiter = " ";
        size_t pos = 0;
        string token;
        int start = 0;
        while ((pos= sp.find(delimiter)) != std::string::npos) {
                start++;
                token = sp.substr(0,pos);
                cout << "G: " << token << endl;
                sp.erase(0, pos + delimiter.length());
		if (start == 2) {
                        if (token.find("CREAT") != std::string::npos) {
                                cd = "create";
                        } else if (token.find("DELET") != std::string::npos) {
                                cd = "delete";
                        }
                }

                if (start == 3)
                        filename = token;
        }
        ip = sp;
	return 0;
}


#define CHAR_SIZE 128

// A Class representing a Trie node
class Trie
{
public:
	bool isLeaf;
	Trie* character[CHAR_SIZE];

	// Constructor
	Trie()
	{
		this->isLeaf = false;

		for (int i = 0; i < CHAR_SIZE; i++)
			this->character[i] = nullptr;
	}

	void insert(std::string);
	bool deletion(Trie*&, std::string);
	bool search(std::string);
	bool haveChildren(Trie const*);
};

Trie *ip_1;
Trie *ip_2;
Trie *ip_3;

// Iterative function to insert a key in the Trie
void Trie::insert(std::string key)
{
	// start from root node
	Trie* curr = this;
	for (int i = 0; i < key.length(); i++)
	{
		// create a new node if path doesn't exists
		if (curr->character[key[i]] == nullptr)
			curr->character[key[i]] = new Trie();

		// go to next node
		curr = curr->character[key[i]];
	}

	// mark current node as leaf
	curr->isLeaf = true;
}

// Iterative function to search a key in Trie. It returns true
// if the key is found in the Trie, else it returns false
bool Trie::search(std::string key)
{
	// return false if Trie is empty
	if (this == nullptr)
		return false;

	Trie* curr = this;
	for (int i = 0; i < key.length(); i++)
	{
		// go to next node
		curr = curr->character[key[i]];

		// if string is invalid (reached end of path in Trie)
		if (curr == nullptr)
			return false;
	}

	// if current node is a leaf and we have reached the
	// end of the string, return true
	return curr->isLeaf;
}

// returns true if given node has any children
bool Trie::haveChildren(Trie const* curr)
{
	for (int i = 0; i < CHAR_SIZE; i++)
		if (curr->character[i])
			return true;	// child found

	return false;
}

// Recursive function to delete a key in the Trie
bool Trie::deletion(Trie*& curr, std::string key)
{
	// return if Trie is empty
	if (curr == nullptr)
		return false;

	// if we have not reached the end of the key
	if (key.length())
	{
		// recurse for the node corresponding to next character in the key
		// and if it returns true, delete current node (if it is non-leaf)

		if (curr != nullptr &&
			curr->character[key[0]] != nullptr &&
			deletion(curr->character[key[0]], key.substr(1)) &&
			curr->isLeaf == false)
		{
			if (!haveChildren(curr))
			{
				delete curr;
				curr = nullptr;
				return true;
			}
			else {
				return false;
			}
		}
	}

	// if we have reached the end of the key
	if (key.length() == 0 && curr->isLeaf)
	{
		// if current node is a leaf node and don't have any children
		if (!haveChildren(curr))
		{
			// delete current node
			delete curr;
			curr = nullptr;

			// delete non-leaf parent nodes
			return true;
		}

		// if current node is a leaf node and have children
		else
		{
			// mark current node as non-leaf node (DON'T DELETE IT)
			curr->isLeaf = false;

			// don't delete its parent nodes
			return false;
		}
	}

	return false;
}


static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size)
{
  size_t i;
  size_t c;
  unsigned int width=0x10;

  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);

  for(i=0; i<size; i+= width) {
    fprintf(stream, "%4.4lx: ", (long)i);

    /* show hex to the left */
    for(c = 0; c < width; c++) {
      if(i+c < size)
        fprintf(stream, "%02x ", ptr[i+c]);
      else
        fputs("   ", stream);
    }

    /* show data on the right */
    for(c = 0; (c < width) && (i+c < size); c++) {
      char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
      fputc(x, stream);
    }

    fputc('\n', stream); /* newline */
  }
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  const char *text;
  (void)handle; /* prevent compiler warning */
  (void)userp;

  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  dump(text, stderr, (unsigned char *)data, size);
  return 0;
}

struct PrintException {
    void operator()(std::exception_ptr exc) const {
        try {
            std::rethrow_exception(exc);
        } catch (const std::exception& e) {
            std::cerr << "An exception occured: " << e.what() << std::endl;
        }
    }
};

struct LoadMonitor {
    LoadMonitor(const std::shared_ptr<Http::Endpoint>& endpoint)
        : endpoint_(endpoint)
        , interval(std::chrono::seconds(1))
    { }

    void setInterval(std::chrono::seconds secs) {
        interval = secs;
    }

    void start() {
        shutdown_ = false;
        thread.reset(new std::thread(std::bind(&LoadMonitor::run, this)));
    }

    void shutdown() {
        shutdown_ = true;
    }

    ~LoadMonitor() {
        shutdown_ = true;
        if (thread) thread->join();
    }

private:
    std::shared_ptr<Http::Endpoint> endpoint_;
    std::unique_ptr<std::thread> thread;
    std::chrono::seconds interval;

    std::atomic<bool> shutdown_;

    void run() {
        Tcp::Listener::Load old;
        while (!shutdown_) {
            if (!endpoint_->isBound()) continue;

            endpoint_->requestLoad(old).then([&](const Tcp::Listener::Load& load) {
                old = load;

                double global = load.global;
                if (global > 100) global = 100;

                if (global > 1)
                    std::cout << "Global load is " << global << "%" << std::endl;
                else
                    std::cout << "Global load is 0%" << std::endl;
            },
            Async::NoExcept);

            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
    }
};


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}




class MyHandler : public Http::Handler {

    HTTP_PROTOTYPE(MyHandler)

    void onRequest(
            const Http::Request& req,
            Http::ResponseWriter response) override {
	std::string res_name = req.resource();
	std::cout << "Orignal request" << res_name << std::endl;
	std::cout << res_name.find("/runLambda") << std::endl;
	if (res_name.find("/runLambda") == 0) {
       		if (req.method() == Http::Method::Post) {
			CURL *curl;
			CURLcode res;
			std::string readBuffer;
			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			cout << "before" << std::endl;
			if (curl) {
				std::string buildurl = "http://128.105.144.125";
				buildurl = buildurl + res_name;
				curl_easy_setopt(curl, CURLOPT_URL, buildurl.c_str());
				string pottno = "8080";
				char * cstr = new char [pottno.length()+1];
  				std::strcpy (cstr, pottno.c_str());
				curl_easy_setopt(curl, CURLOPT_PORT, stoi(cstr));
				string req_body = req.body();
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());//(req.body()).c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				//curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
 				//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
				res = curl_easy_perform(curl);
				if (res != CURLE_OK) {
					fprintf(stderr, "Reason: %s\n", curl_easy_strerror(res));
					response.send(Http::Code::Method_Not_Allowed);
					std::cout << "Method not allowedaaa";
					readBuffer = "Method not allowedbbb";
				}
				//char* url;
				//long response_code;
				//double elapsed;
				/*curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
				curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
				cout << "A" << url << " B: " << response_code << endl;
				*/
				curl_easy_cleanup(curl);
				curl_global_cleanup();
				cout << "Codablcok a" << endl;
           			response.send(Http::Code::Ok, readBuffer, MIME(Text, Plain));
			}
          	} else if (req.method() == Http::Method::Get) {
			CURL *curl;
			CURLcode res;

			curl = curl_easy_init();
			string readBuffer;
			if(curl) {
				curl_easy_setopt(curl, CURLOPT_URL, "http://google.com");
				/* example.com is redirected, so we tell libcurl to follow redirection */
				//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

				curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
				/* Perform the request, res will get the return code */
				res = curl_easy_perform(curl);
				/* Check for errors */
				if(res != CURLE_OK)
					fprintf(stderr, "curl_easy_perform() failed: %s\n",
							curl_easy_strerror(res));

				/* always cleanup */
				curl_easy_cleanup(curl);
			}
			response.send(Http::Code::Ok, readBuffer, MIME(Text, Plain));
		} else {
		//	response.send(Http::Code::Method_Not_Allowed);
            	}
        } else if (req.resource() == "/ping") {
            if (req.method() == Http::Method::Get) {

                using namespace Http;

                auto query = req.query();
                if (query.has("chunked")) {
                    std::cout << "Using chunked encoding" << std::endl;

                    response.headers()
                        .add<Header::Server>("pistache/0.1")
                        .add<Header::ContentType>(MIME(Text, Plain));

                    response.cookies()
                        .add(Cookie("lang", "en-US"));

                    auto stream = response.stream(Http::Code::Ok);
                    stream << "PO";
                    stream << "NG";
                    stream << ends;
                }
                else {
                    response.send(Http::Code::Ok, "PONG");
                }

            }
        }
        else if (req.resource() == "/statistics") {
            if (req.method() == Http::Method::Post) {

                using namespace Http;

                auto query = req.query();
                if (query.has("chunked")) {
                    std::cout << "Using chunked encoding" << std::endl;

                    response.headers()
                        .add<Header::Server>("pistache/0.1")
                        .add<Header::ContentType>(MIME(Text, Plain));

                    response.cookies()
                        .add(Cookie("lang", "en-US"));

                    auto stream = response.stream(Http::Code::Ok);
                    stream << "re";
                    stream << "ce";
                    stream << "iv";
                    stream << "ed";
                    stream << ends;
                }
                else {
                    std::cout << req.body() << std::endl;
                    response.send(Http::Code::Ok, "Received");
                }

            }
        }
        else if (req.resource() == "/register") {
            if (req.method() == Http::Method::Get) {

                using namespace Http;

                auto query = req.query();
                if (query.has("chunked")) {
                    std::cout << "Using chunked encoding" << std::endl;

                    response.headers()
                        .add<Header::Server>("pistache/0.1")
                        .add<Header::ContentType>(MIME(Text, Plain));

                    response.cookies()
                        .add(Cookie("lang", "en-US"));

                    auto stream = response.stream(Http::Code::Ok);
                    //Form chunked uuid and output to stream

		    stream << ends;
                }
                else {

                }

            }
        } else if (req.resource() == "/listoflambda") {
            if (req.method() == Http::Method::Post) {
		cout << "Recv: " << req.body() << endl;;
		string modifyLine = req.body();
		string filename;
		string ip;
		string create_or_delete;
		getFilenameAndIp (modifyLine, filename, ip, create_or_delete);
		cout << "Os: "  << filename << " : " << ip << " : " << create_or_delete << endl;
		if (ip == "10.0.2.15" && create_or_delete == "create")  {
			(ip_1)->insert (filename);
		} 
		if (ip == "10.0.2.15" && create_or_delete == "delete") {
			if (ip_1 != NULL)
				(ip_1)->deletion (ip_1, filename);
		}
 
		if (ip == "" && create_or_delete == "create")  {
                        (ip_2)->insert (filename);
                }
                
		if (ip == "" && create_or_delete == "delete") {
                        if (ip_2 != NULL)
                                (ip_2)->deletion (ip_2, filename);
                }

		if (ip == "" && create_or_delete == "create")  {
                        (ip_3)->insert (filename);
                }
                
		if (ip == "" && create_or_delete == "delete") {
                        if (ip_3 != NULL)
                                (ip_3)->deletion (ip_3, filename);
                }

		cout << "finding" << ip_1->search (filename) << std::endl;

               response.send(Http::Code::Ok, req.body(), MIME(Text, Plain));
            } else {
                response.send(Http::Code::Method_Not_Allowed);
            }
        } else if (req.resource() == "/echo") {
            if (req.method() == Http::Method::Post) {
                response.send(Http::Code::Ok, req.body(), MIME(Text, Plain));
            } else {
                response.send(Http::Code::Method_Not_Allowed);
            }
        }
        else if (req.resource() == "/stream_binary") {
            auto stream = response.stream(Http::Code::Ok);
            char binary_data[] = "some \0\r\n data\n";
            size_t chunk_size = 14;
            for (size_t i = 0; i < 10; ++i) {
                stream.write(binary_data, chunk_size);
                stream.flush();
            }
            stream.ends();
        }
        else if (req.resource() == "/exception") {
            throw std::runtime_error("Exception thrown in the handler");
        }
        else if (req.resource() == "/timeout") {
            response.timeoutAfter(std::chrono::seconds(2));
        }
        else if (req.resource() == "/static") {
            if (req.method() == Http::Method::Get) {
                Http::serveFile(response, "README.md").then([](ssize_t bytes) {
                    std::cout << "Sent " << bytes << " bytes" << std::endl;
                }, Async::NoExcept);
            }
        } else {
            response.send(Http::Code::Not_Found);
        }

    }

    void onTimeout(
            const Http::Request& req,
            Http::ResponseWriter response) override {
        UNUSED(req);
        response
            .send(Http::Code::Request_Timeout, "Timeout")
            .then([=](ssize_t) { }, PrintException());
    }

};

int main(int argc, char *argv[]) {
    Port port(9081);

    int thr = 10;

    if (argc >= 2) {
        port = std::stol(argv[1]);

        if (argc == 3)
            thr = std::stol(argv[2]);
    }

    ip_1 = new Trie();
    ip_2 = new Trie();
    ip_3 = new Trie();

    Address addr(Ipv4::any(), port);

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    auto server = std::make_shared<Http::Endpoint>(addr);

    auto opts = Http::Endpoint::options()
        .threads(thr)
        .flags(Tcp::Options::InstallSignalHandler);
    server->init(opts);
    server->setHandler(Http::make_handler<MyHandler>());
    server->serve();

    std::cout << "Shutdowning server" << std::endl;
    server->shutdown();
}
