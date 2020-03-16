#pragma once
#include "Modules/Tier1.hpp"

#include <cstring>
#include <vector>

#include "Modules/Module.hpp"

#include "Game.hpp"

class CommandBase {
protected:
    ConCommandBase* ptr;

public:
    int version;
    bool isRegistered;
    bool isReference;

public:
    static std::vector<CommandBase*>& GetList();

public:
    CommandBase(int version);
    ~CommandBase();

    virtual bool IsCommand() = 0;
    virtual void Register() = 0;
    virtual void Unregister() = 0;

    inline ConCommandBase* ThisPtr() { return this->ptr; }

    inline bool operator!() { return this->ptr == nullptr; }

    static int RegisterAll();
    static void UnregisterAll();
};

class Command : public CommandBase {
public:
    Command(const char* name);
    Command(const char* pName, _CommandCallback callback, const char* pHelpString, int version = SourceGame_Unknown,
        int flags = 0, _CommandCompletionCallback completionFunc = nullptr);

    bool IsCommand() override { return true; }
    void Register() override;
    void Unregister() override;

    inline ConCommand* ThisPtr() { return reinterpret_cast<ConCommand*>(this->ptr); }

    static bool ActivateAutoCompleteFile(const char* name, _CommandCompletionCallback callback);
    static bool DectivateAutoCompleteFile(const char* name);
};

class CommandHook {
public:
    const char* target;
    _CommandCallback* original;
    _CommandCallback detour;

private:
    bool isRegistered;
    bool isHooked;

public:
    CommandHook(const char* target, _CommandCallback* original, _CommandCallback detour);
    void Register(Module* mod);
    void Unregister();
    void Hook();
    void Unhook();
};

#define CON_COMMAND(name, description)                           \
    void name##_callback(const CCommand& args);                  \
    Command name = Command(#name, name##_callback, description); \
    void name##_callback(const CCommand& args)

#define CON_COMMAND_F(name, description, flags)                                             \
    void name##_callback(const CCommand& args);                                             \
    Command name = Command(#name, name##_callback, description, SourceGame_Unknown, flags); \
    void name##_callback(const CCommand& args)

#define CON_COMMAND_F_COMPLETION(name, description, flags, completion)                                  \
    void name##_callback(const CCommand& args);                                                         \
    Command name = Command(#name, name##_callback, description, SourceGame_Unknown, flags, completion); \
    void name##_callback(const CCommand& args)

#define CON_COMMAND_U(name, description, version)                         \
    void name##_callback(const CCommand& args);                           \
    Command name = Command(#name, name##_callback, description, version); \
    void name##_callback(const CCommand& args)

#define CON_COMMAND_FU(name, description, flags, version)                        \
    void name##_callback(const CCommand& args);                                  \
    Command name = Command(#name, name##_callback, description, version, flags); \
    void name##_callback(const CCommand& args)

#define CON_COMMAND_FU_COMPLETION(name, description, flags, completion, version)             \
    void name##_callback(const CCommand& args);                                              \
    Command name = Command(#name, name##_callback, description, version, flags, completion); \
    void name##_callback(const CCommand& args)

#define DECL_DECLARE_AUTOCOMPLETION_FUNCTION(command) \
    int command##_CompletionFunc(const char* partial, \
        char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
#define DECLARE_AUTOCOMPLETION_FUNCTION(command, subdirectory, extension)            \
    CBaseAutoCompleteFileList command##Complete(#command, subdirectory, #extension); \
    int command##_CompletionFunc(const char* partial,                                \
        char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])  \
    {                                                                                \
        return command##Complete.AutoCompletionFunc(partial, commands);              \
    }

#define AUTOCOMPLETION_FUNCTION(command) \
    command##_CompletionFunc

#define CON_COMMAND_AUTOCOMPLETEFILE(name, description, flags, subdirectory, extension) \
    DECLARE_AUTOCOMPLETION_FUNCTION(name, subdirectory, extension)                      \
    CON_COMMAND_F_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name))

#define CON_COMMAND_AUTOCOMPLETEFILE_U(name, description, flags, subdirectory, extension, version) \
    DECLARE_AUTOCOMPLETION_FUNCTION(name, subdirectory, extension)                                 \
    CON_COMMAND_FU_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name), version)

#define DECL_COMMAND_COMPLETION(command)                \
    DECL_DECLARE_AUTOCOMPLETION_FUNCTION(command)       \
    {                                                   \
        const char* cmd = #command " ";                 \
        char* match = (char*)partial;                   \
        if (std::strstr(partial, cmd) == partial) {     \
            match = match + std::strlen(cmd);           \
        }                                               \
        static auto items = std::vector<std::string>(); \
        items.clear();
#define DECL_AUTO_COMMAND_COMPLETION(command, completion)                      \
    DECL_DECLARE_AUTOCOMPLETION_FUNCTION(command)                              \
    {                                                                          \
        const char* cmd = #command " ";                                        \
        char* match = (char*)partial;                                          \
        if (std::strstr(partial, cmd) == partial) {                            \
            match = match + std::strlen(cmd);                                  \
        }                                                                      \
        static auto items = std::vector<std::string>();                        \
        items.clear();                                                         \
        static auto list = std::vector<std::string> completion;                \
        for (auto& item : list) {                                              \
            if (items.size() == COMMAND_COMPLETION_MAXITEMS) {                 \
                break;                                                         \
            }                                                                  \
            if (std::strlen(match) != std::strlen(cmd)) {                      \
                if (std::strstr(item.c_str(), match)) {                        \
                    items.push_back(item);                                     \
                }                                                              \
            } else {                                                           \
                items.push_back(item);                                         \
            }                                                                  \
        }                                                                      \
        auto count = 0;                                                        \
        for (auto& item : items) {                                             \
            std::strcpy(commands[count++], (std::string(cmd) + item).c_str()); \
        }                                                                      \
        return count;                                                          \
    }
// clang-format off
#define FINISH_COMMAND_COMPLETION()                                        \
    }                                                                      \
    auto count = 0;                                                        \
    for (auto& item : items) {                                             \
        std::strcpy(commands[count++], (std::string(cmd) + item).c_str()); \
    }                                                                      \
    return count;
// clang-format on
#define CON_COMMAND_COMPLETION(name, description, completion) \
    DECL_AUTO_COMMAND_COMPLETION(name, completion)            \
    CON_COMMAND_F_COMPLETION(name, description, 0, name##_CompletionFunc)

#define DETOUR_COMMAND(name)                                                  \
    _CommandCallback name##_callback;                                         \
    void name##_callback_detour(const CCommand& args);                        \
    CommandHook name##_hook(#name, &name##_callback, name##_callback_detour); \
    void name##_callback_detour(const CCommand& args)
