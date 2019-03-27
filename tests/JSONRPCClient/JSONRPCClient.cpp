#define MODULE_NAME JSONRPC_Test

#include <JSONRPCPlugin/Data.h>
#include <core/core.h>
#include <jsonrpc/jsonrpc.h>

using namespace WPEFramework;

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(::JsonValue::type)

    { JsonValue::type::EMPTY,   _TXT("empty")   },
    { JsonValue::type::BOOLEAN, _TXT("boolean") },
    { JsonValue::type::NUMBER, _TXT("number") },
    { JsonValue::type::STRING, _TXT("string") },

ENUM_CONVERSION_END(::JsonValue::type)

ENUM_CONVERSION_BEGIN(Data::Response::state)

    { Data::Response::ACTIVE, _TXT("Activate") },
    { Data::Response::INACTIVE, _TXT("Inactivate") },
    { Data::Response::IDLE, _TXT("Idle") },
    { Data::Response::FAILURE, _TXT("Failure") },

ENUM_CONVERSION_END(Data::Response::state)

}

bool ParseOptions(int argc, char** argv, Core::NodeId& jsonrpcChannel)
{
    int index = 1;
    bool domain = false;
    bool showHelp = false;

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-domain") == 0)
            domain = true;
        else if (strcmp(argv[index], "-h") == 0)
            showHelp = true;
        index++;
    }

    if (!showHelp) {
        if (domain) {
            jsonrpcChannel = Core::NodeId("/tmp/testserver0");
        } else {
            jsonrpcChannel = Core::NodeId("127.0.0.1", 80);
        }
    }

    return (showHelp);
}

void ShowMenu()
{
    printf("Enter\n"
           "\tI : Invoke a synchronous method for getting the server time\n"
           "\tT : Invoke a synchronous method woth aggregated parameters\n"
           "\tR : Register for a-synchronous feedback\n"
           "\tU : Unregister for a-synchronous feedback\n"
           "\tP : Read Property.\n"
           "\t0 : Set property @ value 0.\n"
           "\t1 : Set property @ value 1.\n"
           "\t2 : Set property @ value 2.\n"
           "\t3 : Set property @ value 3.\n"
           "\tW : Read Window Property.\n"
           "\t4 : Set property @ value { 0, 2, 1280, 720 }.\n"
           "\t5 : Set property @ value { 200, 300, 720, 100 }.\n"
           "\tO : Set properties using an opaque variant JSON parameter\n"
           "\tV : Get properties using an opaque variant JSON parameter\n"
           "\tE : Invoke and exchange an opaque variant JSON parameter\n"
           "\tH : Help\n"
           "\tQ : Quit\n");
}

namespace Handlers {

// The methods to be used for handling the incoming methods can be static methods or
// methods in classes. In the client tthere is a demonstration of a static method,
// in the server is a demonstartion of an object method.
// To avoid name clashses it is recomended to put the handlers in a namespace (clock,
// for example, alreay exists in the global namespace and you get very interesting
// compiler warnings if there is a name clash)
static void clock(const Core::JSON::String& parameters)
{
    printf("Received a new time: %s\n", parameters.Value().c_str());
}
}

static void PrintObject(const JsonObject::Iterator& iterator)
{
    JsonObject::Iterator index = iterator;
    while (index.Next() == true) {
        printf("Element [%s]: <%s> = \"%s\"\n",
            index.Label(),
            Core::EnumerateType<JsonValue::type>(index.Current().Content()).Data(),
            index.Current().Value().c_str());
    }
}

int main(int argc, char** argv)
{
    // Additional scoping neede to have a proper shutdown of the STACK object:
    // JSONRPC::Client remoteObject
    {
        Core::NodeId jsonrpcChannel;
        ShowMenu();
        int element;

        ParseOptions(argc, argv, jsonrpcChannel);

        // The JSONRPC Client library is expecting the THUNDER_ACCESS environment variable to be set and pointing
        // to the JSONRPC Server, this can be a domain socket (use at least 1 sleash in it, or a TCP address.
        Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:80")));

        // This is not mandatory, just an easy way to use the VisualStudio environment to Start 2 projects at once, 1 project
        // being the JSONRPC Server running the plugins and the other one this client. However, give the sevrver a bit of time
        // to bring up Plugin JSONRPCPlugin, before we hook up to it. If one starts this App, after the Server is up and running 
        // this is not nessecary.
        SleepMs(2000);

        // Create a remoteObject.  This is the way we can communicate with the Server.
        // The parameters:
        // 1. [mandatory] This is the designator of the module we will connect to.
        // 2. [optional]  This is the designator used for the code we have on my side.
        // 3. [optional]  should the websocket under the hood call directly the plugin
        //                or will it be rlayed through thejsonrpc dispatcher (default,
        //                use jsonrpc dispatcher)
        JSONRPC::Client remoteObject(_T("JSONRPCPlugin.1"), _T("client.events.1"));

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case '0':
            case '1':
            case '2':
            case '3': {
                // Lets trigger some action on server side to get a property, usinng a synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Property to read (See JSONRPCPlugin::JSONRPCPlugin)
                // 3. [mandatory] Parameter that holds the information to "SET" on the other side.
                Core::JSON::String value(string(_T("< ")) + static_cast<char>(element) + string(_T(" >")));
                remoteObject.Set(1000, _T("data"), value);
                break;
            }
            case '4': {
                if (remoteObject.Set<Data::Geometry>(1000, _T("geometry"), 0, 2, 1280, 720) == Core::ERROR_NONE) {
                    printf("Set Window rectangle { 0, 2, 1280, 720 }\n");
                } else {
                    printf("Failed to set Window!\n");
                }
                break;
            }
            case '5': {
                if (remoteObject.Set<Data::Geometry>(1000, _T("geometry"), 200, 300, 720, 100) == Core::ERROR_NONE) {
                    printf("Set Window rectangle { 200, 300, 720, 100 }\n");
                } else {
                    printf("Failed to set Window!\n");
                }
                break;
            }
            case 'W': {
                Data::Geometry window;
                if (remoteObject.Get<Data::Geometry>(1000, _T("geometry"), window) == Core::ERROR_NONE) {
                    printf("Window rectangle { %u, %u, %u, %u}\n", window.X.Value(), window.Y.Value(), window.Width.Value(), window.Height.Value());
                } else {
                    printf("Oopsy daisy, could not get the Geometry parameters!\n");
                }
                break;
            }
            case 'P': {
                // Lets trigger some action on server side to get a property, usinng a synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Property to read (See JSONRPCPlugin::JSONRPCPlugin)
                // 3. [mandatory] Response to be received from the other side.
                Core::JSON::String value;
                remoteObject.Get(1000, _T("data"), value);
                printf("Read propety from the remote object: %s\n", value.Value().c_str());
                break;
            }
            case 'I': {
                // Lets trigger some action on server side to get some feedback. The regular synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Method name to call (See JSONRPCPlugin::JSONRPCPlugin - 14)
                // 3. [mandatory] Parameters to be send to the other side.
                // 4. [mandatory] Response to be received from the other side.
                Core::JSON::String result;
                remoteObject.Invoke<Core::JSON::String, Core::JSON::String>(1000, _T("time"), _T(""), result);
                printf("received time: %s\n", result.Value().c_str());
                break;
            }
            case 'T': {
                // Lets trigger some action on server side to get some feedback. The regular synchronous RPC call.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Method name to call (See JSONRPCPlugin::JSONRPCPlugin - 14)
                // 3. [mandatory] Parameters to be send to the other side.
                // 4. [mandatory] Response to be received from the other side.
                Data::Response response;
                remoteObject.Invoke<Data::Parameters, Data::Response>(1000, _T("extended"), Data::Parameters(_T("JustSomeText"), true), response);
                printf("received time: %ju - %d\n", (intmax_t)response.Time.Value(), response.State.Value());
                break;
            }
            case 'R': {
                // We have a handler, called Handlers::clock to handle the events coming from the Server.
                // If we register this handler, it will also automatically be register this handler on the server side.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Event name to subscribe to on server side (See JSONRPCPlugin::SendTime - 44)
                // 3. [mandatory] Code to handle this event, it is allowed to use a lambda here, or a object method (see plugin)
                if (remoteObject.Subscribe<Core::JSON::String>(1000, _T("clock"), &Handlers::clock) == Core::ERROR_NONE) {
                    printf("Installed a notification handler and registered for the notifications\n");
                } else {
                    printf("Failed to install a notification handler\n");
                }
                break;
            }
            case 'U': {
                // We are no longer interested inm the events, ets get ride of the notifications.
                // The parameters:
                // 1. [mandatory] Time to wait for the round trip to complete to the server to register.
                // 2. [mandatory] Event name which was used during the registration
                remoteObject.Unsubscribe(1000, _T("clock"));
                printf("Unregistered and renmoved a notification handler\n");
                break;
            }
            case 'O': {
				// Set properties, using a n Opaque value:
                remoteObject.SetProperty("window", {{{ "x", 123 }, { "y", 456 }, { "width", 789 }, { "height", 1112 }}});
                printf("Send out an opaque struct to set the window properties\n");
                break;
            }
            case 'V': {
                // Get properties, using an Opaque value:
                JsonObject result;
                remoteObject.GetProperty("window", result);
                JsonValue value(result.Get("x"));
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<x> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {
                    printf("<x> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<x>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                value = result.Get("y");
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<y> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {
                    printf("<y> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<y>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                value = result.Get("width");
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<width> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {
                    printf("<width> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<width>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                value = result.Get("height");
                if (value.Content() == JsonValue::type::EMPTY) {
                    printf("<height> value not available\n");
                } else if (value.Content() != JsonValue::type::NUMBER) {
                    printf("<height> is expected to be a number but it is: %s\n", Core::EnumerateType<JsonValue::type>(value.Content()).Data());
                } else {
                    printf("<height>: %d\n", static_cast<uint32_t>(value.Number()));
                }
                break;
            }
            case 'E': {
				// Set this one up, the old fasion way
                JsonObject parameters;
                JsonObject response;
                parameters["x"] = 67;
                parameters["y"] = 99;
                parameters["width"] = false;  // Deliberate error, see what happens..
                parameters["height"] = -1299; // Interesting a negative hide... No one is warning us :-)
                PrintObject(parameters.Variants());
                if (remoteObject.Invoke("swap", parameters, response) == Core::ERROR_NONE) {
                    PrintObject(response.Variants());
				} else {
                    printf("Something went wrong during the invoke\n");                
				}

                break;
			}
            case '?':
            case 'H':
                ShowMenu();
            }

        } while (element != 'Q');
    }

    printf("Leaving app.\n");

    Core::Singleton::Dispose();

    return (0);
}
