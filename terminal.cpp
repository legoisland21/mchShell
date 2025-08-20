#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fstream>
using namespace std;

vector<string> blockedCommands = {"help", "ff"};
vector<string> customCommands = {"ff"};

// Custom commands

void findFilesAndFolders(const string& searchTerm) {
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA("*", &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        cout << "Nothing found" << endl;
        return;
    }

    vector<string> files;
    vector<string> folders;

    do {
        string name = fd.cFileName;

        if (name == "." || name == "..") continue;

        // only match beginning
        if (name.substr(0, searchTerm.size()) == searchTerm) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                folders.push_back(name);
            else
                files.push_back(name);
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);

    if (!files.empty()) {
        cout << "FILES:" << endl;
        for (auto &f : files) cout << f << " ";
        cout << endl;
    }

    if (!folders.empty()) {
        cout << "FOLDERS:" << endl;
        for (auto &d : folders) cout << d << " ";
        cout << endl;
    }

    if (files.empty() && folders.empty())
        cout << "No files/folders found" << endl;
}

// Environment Variable handler

string getEnvironmentVariable(const string& name) {
    char buffer[1024];
    DWORD result = GetEnvironmentVariableA(name.c_str(), buffer, sizeof(buffer));
    
    if (result == 0) return "";
    else if (result > sizeof(buffer)) {
        vector<char> largeBuffer(result);
        GetEnvironmentVariableA(name.c_str(), largeBuffer.data(), result);
        return string(largeBuffer.data());
    }
    
    return string(buffer);
}

string expandEnvironmentVariables(const string& input) {
    string result = input;
    size_t startPos = 0;
    
    while ((startPos = result.find('%', startPos)) != string::npos) {
        size_t endPos = result.find('%', startPos + 1);
        if (endPos == string::npos) break;
        
        string varName = result.substr(startPos + 1, endPos - startPos - 1);
        string varValue = getEnvironmentVariable(varName);
        
        if (!varValue.empty()) { result.replace(startPos, endPos - startPos + 1, varValue); startPos += varValue.length(); } 
        else startPos = endPos + 1;
    }
    
    return result;
}

// Command handling

bool openProcess(string app, string params) {
    HINSTANCE h = ShellExecuteA(nullptr, "open", app.c_str(), params.c_str(), nullptr, SW_SHOWNORMAL);

    if ((INT_PTR)h <= 32) return false;
    else return true;
}

string outputParams(string input) {
    size_t pos = input.find(' ');
    string cmd;
    string args;
    if (pos == string::npos) { cmd = input; args = ""; }
    else { cmd = input.substr(0, pos); args = input.substr(pos + 1); }
    return args;
}

string outputApp(string input) {
    size_t pos = input.find(' ');
    string cmd;
    string args;
    if (pos == string::npos) { cmd = input; args = "";}
    else { cmd = input.substr(0, pos); args = input.substr(pos + 1); }
    return cmd;
}

// Folder/File handling

bool directoryExists(const string& path) {
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool fileExistsWOX(const string& filenameBase) {
    vector<string> extensions = {".exe", ".bat", ".cmd", ".dll"};
    for (const auto& ext : extensions) {
        string fullPath = filenameBase + ext;
        DWORD attrib = GetFileAttributesA(fullPath.c_str());
        if (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY)) return true;
    }
    return false;
}

bool changeDirectory(const string& input) {
    string target = input;

    size_t start = target.find_first_not_of(" ");
    size_t end = target.find_last_not_of(" ");
    if (start != string::npos && end != string::npos) target = target.substr(start, end - start + 1);
    
    if (!target.empty() && target.front() == '"' && target.back() == '"') target = target.substr(1, target.size() - 2);

    target = expandEnvironmentVariables(target);

    if (!directoryExists(target)) {
        cout << "This path doesn't exist: " << target << endl;
        return false;
    }

    return SetCurrentDirectoryA(target.c_str()) != 0;
}

string getPath() {
    char buffer[MAX_PATH];
    DWORD len = GetCurrentDirectoryA(MAX_PATH, buffer);

    if (len == 0) return "";

    return string(buffer);
}

bool isInPath(const string &program) {
    const char* envPath = getenv("PATH");
    if (!envPath) return false;

    vector<string> extensions = {".exe", ".bat", ".cmd"};
    stringstream ss(envPath);
    string dir;

    while (getline(ss, dir, ';')) {
        for (const auto& ext : extensions) {
            string fullPath = dir + "\\" + program + ext;
            DWORD attrib = GetFileAttributesA(fullPath.c_str());
            if (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY)) return true;
        }
    }
    return false;
}

// Misc

bool checkApp(const string& command, const vector<string>& names) { return find(names.begin(), names.end(), command) == names.end(); }

// Get PC Info

string getCurrentUser() {
    char username[128];
    DWORD size = sizeof(username);
	if (GetUserNameA(username, &size)) return string(username);
    else return "UnknownUser";
}

string getPCName() {
    char name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(name);

    if (GetComputerNameA(name, &size)) return string(name);
    else return "UnknownPC";
}

// Ascii Art

void printSplash() {
    cout << R"(                 _      _____ _          _ _  )" << endl;
    cout << R"(                | |    / ____| |        | | | )" << endl;
    cout << R"(  _ __ ___   ___| |__ | (___ | |__   ___| | | )" << endl;
    cout << R"( | '_ ` _ \ / __| '_ \ \___ \| '_ \ / _ \ | | )" << endl;
    cout << R"( | | | | | | (__| | | |____) | | | |  __/ | | )" << endl;
    cout << R"( |_| |_| |_|\___|_| |_|_____/|_| |_|\___|_|_| )" << endl;
}

// Terminal Settings

void setColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// MCH

void runMCH(const string &file) {
    ifstream mch(file);
    if (!mch.is_open()) {
        cerr << "Could not open " << file << endl;
        return;
    }

    string line;
    while (getline(mch, line)) {
        if (line.empty()) continue;
        if (line.substr(0, 3) == "ff ") {
            findFilesAndFolders(line.substr(3));
        } else if (line.substr(0, 3) == "cd ") {
            changeDirectory(line.substr(3));
        } else {
            system(line.c_str());
        }
    }
}

void runAutoexec() {
    ifstream autoexec("autoexec.mch");
    if(!autoexec.is_open()) return;

    string line;
    while(getline(autoexec, line)) {
        if(line.empty() || line[0] == '#') continue;
        system(line.c_str());
    }
}

int main() {
    runAutoexec();
    string command;
    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printSplash();

    cout << "Welcome to mchShell, " << getCurrentUser() << "@" << getPCName() << "!" << endl;

    while(true) {
        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE);
        cout << "[" << getCurrentUser() << "@" << getPCName() << " " << getPath() << "]";
        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        cout << " $ ";

        getline(cin, command);

        if (command == "exit") break;

        if (command.substr(0, 3) == "ff ") {
            string name = command.substr(3);
            findFilesAndFolders(name);
            continue;
        }

        if (command == "cd..") command = "cd ..";

        if (command.substr(0, 3) == "cd ") {
            string newDir = command.substr(3);
            changeDirectory(newDir);
            continue;
        }

        if (command.substr(0, 5) == "smch ") {
            runMCH(command.substr(5));
            continue;
        }

        if (!checkApp(command, blockedCommands)) {
            system(command.c_str());
            cout << endl;
            continue;
        }

        string app = outputApp(command);
        string params = outputParams(command);

        if ((fileExistsWOX(app) && checkApp(app, blockedCommands)) || (isInPath(app) && checkApp(app, blockedCommands) && fileExistsWOX(app))) {
            if(!openProcess(app, params)) cerr << "Error opening app" << endl;
        }
        else system(command.c_str());

        cout << endl;
    }
}