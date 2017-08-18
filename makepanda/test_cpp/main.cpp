#include <pandaSystem.h>
#include <pandaFramework.h>
#include <loader.h>
#include <virtualFileSystem.h>
#include <nodePath.h>

#define SUCCESS_TEST 0
#define FAILURE_DO_FRAME 1
#define FAILURE_LOAD_SYSTEMS 2

int main(int argc, char* argv[])
{
    PandaFramework framework;
    framework.open_framework(argc, argv);
    framework.set_window_title("Panda3D Test");

    for (int i=0; i < 10; ++i)
    {
        if (!framework.do_frame(Thread::get_current_thread()))
            return FAILURE_DO_FRAME;
    }

    if (!(PandaSystem::get_global_ptr() &&
        VirtualFileSystem::get_global_ptr() &&
        Loader::get_global_ptr()))
        return FAILURE_LOAD_SYSTEMS;

    framework.close_framework();

    return SUCCESS_TEST;
}
