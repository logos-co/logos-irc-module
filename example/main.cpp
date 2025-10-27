#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <iostream>
#include <QCommandLineParser>
#include <QPluginLoader>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QMetaEnum>
#include <QObject>
#include <QtPlugin>
#include <QString>
#include <string>

// Import the C API from liblogos_core
extern "C" {
    void logos_core_init(int argc, char *argv[]);
    void logos_core_set_plugins_dir(const char* plugins_dir);
    void logos_core_start();
    int logos_core_exec();
    void logos_core_cleanup();
    char** logos_core_get_loaded_plugins();
    char** logos_core_get_known_plugins();
    int logos_core_load_plugin(const char* plugin_name);
    char* logos_core_process_plugin(const char* plugin_path);
    
    // Callback type for async operations
    typedef void (*AsyncCallback)(int result, const char* message, void* user_data);
    void logos_core_call_plugin_method_async(const char* plugin_name, const char* method_name, const char* params_json, AsyncCallback callback, void* user_data);
}

// Minimal local declaration of the PluginInterface with the same IID
class PluginInterface
{
public:
    virtual ~PluginInterface() {}
    virtual QString name() const = 0;
    virtual QString version() const = 0;
};

#define PluginInterface_iid "com.example.PluginInterface"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

// Helper function to convert C-style array to QStringList
QStringList convertPluginsToStringList(char** plugins) {
    QStringList result;
    if (plugins) {
        for (int i = 0; plugins[i] != nullptr; i++) {
            result.append(plugins[i]);
        }
    }
    return result;
}

// Function to inspect plugin methods using QPluginLoader
void inspectPluginMethods(const QString& pluginPath) {
    std::cout << "\n=== Inspecting Plugin: " << pluginPath.toStdString() << " ===" << std::endl;
    
    QPluginLoader loader(pluginPath);
    QObject *plugin = loader.instance();

    if (!plugin) {
        std::cout << "✗ Failed to load plugin: " << loader.errorString().toStdString() << std::endl;
        return;
    }

    std::cout << "✓ Plugin loaded successfully." << std::endl;

    // Try to cast to PluginInterface
    if (PluginInterface *iface = qobject_cast<PluginInterface *>(plugin)) {
        std::cout << "Plugin name: " << iface->name().toStdString() << std::endl;
        std::cout << "Plugin version: " << iface->version().toStdString() << std::endl;
    } else {
        const QMetaObject *meta = plugin->metaObject();
        if (meta) {
            std::cout << "Qt class name: " << meta->className() << std::endl;
        }
    }

    // List all available methods in the plugin
    const QMetaObject *meta = plugin->metaObject();
    if (meta) {
        std::cout << "\n=== Available Methods ===" << std::endl;
        
        // List all methods (including inherited ones)
        for (int i = 0; i < meta->methodCount(); ++i) {
            QMetaMethod method = meta->method(i);
            QString methodType;
            
            switch (method.methodType()) {
                case QMetaMethod::Method:
                    methodType = "Method";
                    break;
                case QMetaMethod::Signal:
                    methodType = "Signal";
                    break;
                case QMetaMethod::Slot:
                    methodType = "Slot";
                    break;
                case QMetaMethod::Constructor:
                    methodType = "Constructor";
                    break;
                default:
                    methodType = "Unknown";
                    break;
            }
            
            QString accessLevel;
            if (method.access() == QMetaMethod::Public) {
                accessLevel = "Public";
            } else if (method.access() == QMetaMethod::Protected) {
                accessLevel = "Protected";
            } else if (method.access() == QMetaMethod::Private) {
                accessLevel = "Private";
            } else {
                accessLevel = "Unknown";
            }
            
            std::cout << "  " << accessLevel.toStdString() << " " << methodType.toStdString() 
                      << " " << method.name().constData() << "(" 
                      << method.parameterNames().join(", ").toStdString() << ")" << std::endl;
        }
        
        std::cout << "\n=== Properties ===" << std::endl;
        for (int i = 0; i < meta->propertyCount(); ++i) {
            QMetaProperty prop = meta->property(i);
            std::cout << "  Property: " << prop.name() 
                      << " (" << prop.typeName() << ")" << std::endl;
        }
        
        std::cout << "\n=== Enums ===" << std::endl;
        for (int i = 0; i < meta->enumeratorCount(); ++i) {
            QMetaEnum enumerator = meta->enumerator(i);
            std::cout << "  Enum: " << enumerator.name() << std::endl;
            for (int j = 0; j < enumerator.keyCount(); ++j) {
                std::cout << "    " << enumerator.key(j) << " = " << enumerator.value(j) << std::endl;
            }
        }
    }
    
    std::cout << "===============================================" << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "=== Logos IRC Module Example ===" << std::endl;
    
    // Parse command line arguments BEFORE initializing logos core
    QCoreApplication app(argc, argv);
    app.setApplicationName("logos-irc-example");
    app.setApplicationVersion("1.0.0");
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Logos IRC Module Example - Loads IRC module with dependencies");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption modulePathOption(QStringList() << "m" << "module-path",
                                       "Path to the modules directory",
                                       "path");
    parser.addOption(modulePathOption);
    
    parser.process(app);
    
    // Determine plugins directory
    QString pluginsDir;
    if (parser.isSet(modulePathOption)) {
        pluginsDir = QDir::cleanPath(parser.value(modulePathOption));
        std::cout << "Using custom module path: " << pluginsDir.toStdString() << std::endl;
    } else {
        pluginsDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../modules");
        std::cout << "Using default module path: " << pluginsDir.toStdString() << std::endl;
    }
    
    // Initialize logos core (but don't create another QCoreApplication)
    logos_core_init(0, nullptr);  // Pass 0, nullptr since we already have QCoreApplication
    std::cout << "Logos Core initialized" << std::endl;
    
    std::cout << "Setting plugins directory to: " << pluginsDir.toStdString() << std::endl;
    logos_core_set_plugins_dir(pluginsDir.toUtf8().constData());
    
    // Start the core (this discovers and processes plugins)
    logos_core_start();
    std::cout << "Logos Core started successfully!" << std::endl;
    
    // Get and display known plugins
    char** knownPlugins = logos_core_get_known_plugins();
    QStringList knownList = convertPluginsToStringList(knownPlugins);
    
    std::cout << "\n=== Known Plugins ===" << std::endl;
    if (knownList.isEmpty()) {
        std::cout << "No plugins found." << std::endl;
    } else {
        std::cout << "Found " << knownList.size() << " plugin(s):" << std::endl;
        foreach (const QString &plugin, knownList) {
            std::cout << "  - " << plugin.toStdString() << std::endl;
        }
    }
    
    // Determine plugin extension based on platform
    QString pluginExtension;
#if defined(Q_OS_MAC)
    pluginExtension = ".dylib";
#elif defined(Q_OS_WIN)
    pluginExtension = ".dll";
#else // Linux and others
    pluginExtension = ".so";
#endif
    
    // Load modules in the specified order
    std::cout << "\n=== Loading Modules ===" << std::endl;
    
    // 1. Load capability_module
    QString capabilityModulePath = pluginsDir + "/capability_module_plugin" + pluginExtension;
    std::cout << "Processing capability_module plugin from: " << capabilityModulePath.toStdString() << std::endl;
    logos_core_process_plugin(capabilityModulePath.toUtf8().constData());
    inspectPluginMethods(capabilityModulePath);
    
    bool loaded = logos_core_load_plugin("capability_module");
    if (loaded) {
        std::cout << "✓ capability_module plugin loaded successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to load capability_module plugin" << std::endl;
    }
    
    // 2. Load waku_module (load only, no init)
    QString wakuModulePath = pluginsDir + "/waku_module_plugin" + pluginExtension;
    std::cout << "Processing waku_module plugin from: " << wakuModulePath.toStdString() << std::endl;
    logos_core_process_plugin(wakuModulePath.toUtf8().constData());
    inspectPluginMethods(wakuModulePath);
    
    loaded = logos_core_load_plugin("waku_module");
    if (loaded) {
        std::cout << "✓ waku_module plugin loaded successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to load waku_module plugin" << std::endl;
    }
    
    // 3. Load chat_module
    QString chatModulePath = pluginsDir + "/chat_plugin" + pluginExtension;
    std::cout << "Processing chat_plugin from: " << chatModulePath.toStdString() << std::endl;
    logos_core_process_plugin(chatModulePath.toUtf8().constData());
    inspectPluginMethods(chatModulePath);
    
    loaded = logos_core_load_plugin("chat");
    if (loaded) {
        std::cout << "✓ chat plugin loaded successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to load chat plugin" << std::endl;
    }
    
    // 4. Load logos_irc_plugin
    QString ircModulePath = pluginsDir + "/logos_irc_plugin" + pluginExtension;
    std::cout << "Processing logos_irc_plugin from: " << ircModulePath.toStdString() << std::endl;
    logos_core_process_plugin(ircModulePath.toUtf8().constData());
    inspectPluginMethods(ircModulePath);
    
    loaded = logos_core_load_plugin("logos_irc");
    if (loaded) {
        std::cout << "✓ logos_irc plugin loaded successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to load logos_irc plugin" << std::endl;
    }
    
    // Get and display loaded plugins
    char** loadedPlugins = logos_core_get_loaded_plugins();
    QStringList loadedList = convertPluginsToStringList(loadedPlugins);
    
    std::cout << "\n=== Loaded Plugins ===" << std::endl;
    if (loadedList.isEmpty()) {
        std::cout << "No plugins loaded." << std::endl;
    } else {
        std::cout << "Currently loaded " << loadedList.size() << " plugin(s):" << std::endl;
        foreach (const QString &plugin, loadedList) {
            std::cout << "  - " << plugin.toStdString() << std::endl;
        }
    }
    
    std::cout << "\n=== Running Event Loop ===" << std::endl;
    std::cout << "Press Ctrl+C to exit..." << std::endl;
    
    // Run the event loop
    int result = logos_core_exec();
    
    // Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    logos_core_cleanup();
    
    return result;
}
