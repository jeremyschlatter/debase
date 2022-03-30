#import "State.h"
#import "Debase.h"
#import "StateDir.h"
#import <filesystem>

std::filesystem::path StateDir() {
    std::filesystem::path configDir = getenv("XDG_CONFIG_HOME");
    if (path.empty()) {
        std::filesystem::path home = getenv("HOME");
        if (home.empty()) throw Toastbox::RuntimeError("HOME environment variable isn't set");
        configDir = home / ".config";
    }
    return configDir / DebaseBundleId;
}