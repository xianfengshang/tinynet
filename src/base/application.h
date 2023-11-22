// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <list>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include "at_exit.h"

//Global namespace tinynet
namespace tinynet {
class ModuleBase;

typedef ModuleBase* ModulePtr;

typedef std::function<int(void)> ModuleCall;

typedef std::function<int(int)> SysSigHandler;

typedef std::function<void()> ModuleEntry;

typedef std::vector<ModuleEntry> ModuleEntries;
ModuleEntries& GetModuleEntries();

//Module descriptor
struct ModuleItem {
    ModulePtr module; //ModuleBase pointer
    int priority;	  //Startup priority value
    ModuleCall Init;  //Module initialize function
    ModuleCall Start; //Module startup function
    ModuleCall Stop;  //Module stop function
    ModuleCall Run;   //Module run function
    SysSigHandler Signal;//Module signal handle function
    //Equal operator
    bool operator == (const ModuleItem& o) { return this->module == o.module; }
};

//Store command line args
struct CommandLineArgs {
    int argc{ 0 };
    char ** argv{ nullptr };
};


//Application class
class Application {
  public:
    //Application initialize
    int Init();

    //Application start
    int Start();

    //Application stop
    int Stop();

    //Application run
    int Run();

    //Signal handler
    int Signal(int signum);

    //Application loop
    int RunForever();

    //Register  module
    bool RegisterModule(const ModuleItem& item);

    //Set command line args
    void SetArgs(int argc, char* argv[]);
  public:
    //A copy of command line args
    typedef std::vector<std::string> Args;

    const Args& GetArgs() const { return args_; }

    const CommandLineArgs& GetCommandLineArgs() const { return cmd_line_args_; }

    int get_argv_space_size() { return argv_space_size_; }

    //Application shutdown
    void Shutdown();

    bool is_daemon() { return daemonize_; }
  private:
    void ParseCommandLine();
    void InstallSignalHandlers();
    void Daemonize();
    void CreatePidFile();
    void MigrateEnviron();
  public:
    typedef std::list<ModuleItem> ModuleItemList;
  public:
    Application();
    ~Application() = default;
  private:
    Args args_;	//A copy of command line args
    CommandLineArgs cmd_line_args_; //Original command line args
    ModuleItemList modules_;
    AtExitManager at_exit_;//At exit manager
    std::atomic_bool shutdown_; //Shutdown flag
    bool daemonize_;///Daemonize flag
    std::string pidfile_;//pid file path
    std::unique_ptr<char[]> migrated_environ_;
    int argv_space_size_;
};
}

//The global application pointer
extern tinynet::Application* g_App;
