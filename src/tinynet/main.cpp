// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "base/application.h"
int main(int argc, char* argv[]) {
    g_App->SetArgs(argc, argv);
    int code = g_App->Init();
    if (code != 0) {
        return code;
    }
    code = g_App->Start();
    if (code != 0) {
        return code;
    }
    g_App->RunForever();
    code = g_App->Stop();
    return code;
}
