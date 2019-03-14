#include "Module.h"

#include <wpe/webkit-web-extension.h>

#include <cstdio>
#include <memory>
#include <syslog.h>

using namespace WPEFramework;

static Core::NodeId GetConnectionNode() {
    string nodeName;

    Core::SystemInfo::GetEnvironment(string(_T("COMMUNICATOR_CONNECTOR")), nodeName);

    return (Core::NodeId (nodeName.c_str()));
}

static class PluginHost 
{
    private:
        PluginHost(const PluginHost&) = delete;
        PluginHost& operator= (const PluginHost&) = delete;

    public:
        PluginHost()
            : _comClient(Core::ProxyType< RPC::CommunicatorClient >::Create(GetConnectionNode(), Core::ProxyType< RPC::InvokeServerType<16,2> >::Create() ) )
        {
        }
        ~PluginHost()
        {
            TRACE_L1("Destructing injected bundle stuff!!! [%d]", __LINE__);
            Deinitialize();
        }

    public:
        void Initialize (WebKitWebExtension* extension, GVariant* userData)
        {

            Trace::TraceType<Trace::Information, &Core::System::MODULE_NAME>::Enable(true);

            // We have something to report back, do so...
            uint32_t result = _comClient->Open(RPC::CommunicationTimeOut);
            if ( result != Core::ERROR_NONE ) { 
                TRACE(Trace::Error, (_T("Could not open connection to node %s. Error: %s"), _comClient  ->Source().RemoteId(), Core::NumberType<uint32_t>(result).Text()));
            }
            else {
                _comClient.Release();
            }

            _extension = WEBKIT_WEB_EXTENSION(g_object_ref(extension));

            const char* uid;
            const char* whitelist;
            g_variant_get(userData, "(&sm&s)", &uid, &whitelist);

            _scriptWorld = webkit_script_world_new_with_name(uid);
            g_signal_connect(_scriptWorld, "window-object-cleared", G_CALLBACK (windowObjectClearedCallback), nullptr);

            g_signal_connect(extension, "page-created", G_CALLBACK(pageCreatedCallback), this);

            if (whitelist)
                addOriginAccessWhiteList(whitelist);
        }

        void Deinitialize() 
        {
            if( _comClient.IsValid() == true ) {
                _comClient.Release();
            }

            g_object_unref(_scriptWorld);
            g_object_unref(_extension);

	    Core::Singleton::Dispose();
        }

    private:
        static void automationMilestone(const char* arg1, const char* arg2, const char* arg3)
        {
            g_printerr("TEST TRACE: \"%s\" \"%s\" \"%s\"\n", arg1, arg2, arg3);
            TRACE_GLOBAL(Trace::Information, (_T("TEST TRACE: \"%s\" \"%s\" \"%s\""), arg1, arg2, arg3));
        }

        static void windowObjectClearedCallback(WebKitScriptWorld* world, WebKitWebPage*, WebKitFrame* frame)
        {
            if (!webkit_frame_is_main_frame(frame))
                return;

            JSCContext* jsContext = webkit_frame_get_js_context_for_script_world(frame, world);

            JSCValue* automation = jsc_value_new_object(jsContext, nullptr, nullptr);
            JSCValue* function = jsc_value_new_function(jsContext, nullptr, reinterpret_cast<GCallback>(automationMilestone),
                nullptr, nullptr, G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
            jsc_value_object_set_property(automation, "Milestone", function);
            g_object_unref(function);
            jsc_context_set_value(jsContext, "automation", automation);
            g_object_unref(automation);

            static const char wpeNotifyWPEFramework[] = "var wpe = {};\n"
                "wpe.NotifyWPEFramework = function() {\n"
                "  let retval = new Array;\n"
                "  for (let i = 0; i < arguments.length; i++) {\n"
                "    retval[i] = arguments[i];\n"
                "  }\n"
                "  window.webkit.messageHandlers.wpeNotifyWPEFramework.postMessage(retval);\n"
                "}";
            JSCValue* result = jsc_context_evaluate(jsContext, wpeNotifyWPEFramework, -1);
            g_object_unref(result);

            g_object_unref(jsContext);
        }

        static void pageCreatedCallback(WebKitWebExtension*, WebKitWebPage* page, PluginHost* host)
        {
            // Enforce the creation of the script world global context in the main frame.
            JSCContext* jsContext = webkit_frame_get_js_context_for_script_world(webkit_web_page_get_main_frame(page), host->_scriptWorld);
            g_object_unref(jsContext);

            g_signal_connect(page, "console-message-sent", G_CALLBACK(consoleMessageSentCallback), nullptr);
        }

        static void consoleMessageSentCallback(WebKitWebPage* page, WebKitConsoleMessage* message)
        {
            string messageString = Core::ToString(webkit_console_message_get_text(message));

            const uint16_t maxStringLength = Trace::TRACINGBUFFERSIZE - 1;
            if (messageString.length() > maxStringLength) {
                messageString = messageString.substr(0, maxStringLength);
            }

            // TODO: use "Trace" classes for different levels.
            TRACE_GLOBAL(Trace::Information, (messageString));
        }

        void addOriginAccessWhiteList(const char* whitelist)
        {
            class JSONEntry : public Core::JSON::Container {
            private:
                JSONEntry& operator=(const JSONEntry&) = delete;

            public:
                JSONEntry()
                    : Core::JSON::Container()
                    , Origin()
                    , Domain()
                    , SubDomain(true)
                {
                    Add(_T("origin"), &Origin);
                    Add(_T("domain"), &Domain);
                    Add(_T("subdomain"), &SubDomain);
                }

                JSONEntry(const JSONEntry& rhs)
                    : Core::JSON::Container()
                    , Origin(rhs.Origin)
                    , Domain(rhs.Domain)
                    , SubDomain(rhs.SubDomain)
                {
                    Add(_T("origin"), &Origin);
                    Add(_T("domain"), &Domain);
                    Add(_T("subdomain"), &SubDomain);
                }

            public:
                Core::JSON::String Origin;
                Core::JSON::ArrayType<Core::JSON::String> Domain;
                Core::JSON::Boolean SubDomain;
            };

            Core::JSON::ArrayType<JSONEntry> entries;
            entries.FromString(Core::ToString(whitelist));
            Core::JSON::ArrayType<JSONEntry>::Iterator originIndex(entries.Elements());

            while (originIndex.Next() == true) {
                if ((originIndex.Current().Origin.IsSet() == true) && (originIndex.Current().Domain.IsSet() == true)) {
                    WebKitSecurityOrigin* origin = webkit_security_origin_new_for_uri(originIndex.Current().Origin.Value().c_str());
                    gboolean allowSubdomains = originIndex.Current().SubDomain.Value();

                    Core::JSON::ArrayType<Core::JSON::String>::Iterator domainIndex(originIndex.Current().Domain.Elements());
                    while (domainIndex.Next()) {
                        WebKitSecurityOrigin* domain = webkit_security_origin_new_for_uri(domainIndex.Current().Value().c_str());
                        webkit_web_extension_add_origin_access_whitelist_entry(_extension, origin,
                            webkit_security_origin_get_protocol(domain), webkit_security_origin_get_host(domain),
                            allowSubdomains);
                        webkit_security_origin_unref(domain);
                        TRACE_L1("Added origin->domain pair to WebKit white list: %s -> %s", originIndex.Current().Origin.Value().c_str(), domainIndex.Current().Value().c_str());
                    }

                    webkit_security_origin_unref(origin);
                }
            }
        }

        Core::ProxyType<RPC::CommunicatorClient> _comClient;

        WebKitWebExtension* _extension;
        WebKitScriptWorld* _scriptWorld;

} _wpeFrameworkClient;

extern "C" {

__attribute__((destructor))
static void unload() {
    _wpeFrameworkClient.Deinitialize();
}

// Declare module name for tracer.
MODULE_NAME_DECLARATION(BUILD_REFERENCE)


G_MODULE_EXPORT void webkit_web_extension_initialize_with_user_data(WebKitWebExtension* extension, GVariant* userData)
{
    _wpeFrameworkClient.Initialize(extension, userData);
}

}
