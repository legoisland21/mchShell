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

void testNet() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    vector<string> sites = {
        "google.com",
        "yahoo.com",
        "example.com",
        "wikipedia.org",
        "bing.com",
        "duckduckgo.com"
    };

    for (const auto &site : sites) {
        string cmd = "ping -n 1 " + site;
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (!pipe) {
            SetConsoleTextAttribute(hConsole, 12);
            cout << "error pinging site: " << site << endl;
            SetConsoleTextAttribute(hConsole, 7);
            continue;
        }

        char buffer[128];
        bool success = false;
        while (fgets(buffer, sizeof(buffer), pipe)) {
            string line(buffer);
            if (line.find("TTL=") != string::npos) {
                success = true;
                break;
            }
        }

        _pclose(pipe);

        if (success) {
            SetConsoleTextAttribute(hConsole, 10);
            cout << site << " is reachable!" << endl;
        } else {
            SetConsoleTextAttribute(hConsole, 12);
            cout << "error pinging site: " << site << endl;
        }
        SetConsoleTextAttribute(hConsole, 7);
    }
}

void findInFile(const string &filename, const string &search) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open " << filename << endl;
        return;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    string line;
    int lineNumber = 1;

    while (getline(file, line)) {
        size_t pos = 0;
        bool found = false;
        while ((pos = line.find(search, pos)) != string::npos) {
            found = true;
            cout << filename << ":" << lineNumber << ">";
            cout << line.substr(0, pos);

            SetConsoleTextAttribute(hConsole, 14);
            cout << search;

            SetConsoleTextAttribute(hConsole, 7);
            cout << line.substr(pos + search.length()) << endl;

            pos += search.length();
        }
        if (!found) lineNumber++;
        else lineNumber++;
    }
}

uint64_t getFileOrFolderSize(const string &path) {
    WIN32_FIND_DATAA fd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    uint64_t totalSize = 0;

    string searchPath = path + "\\*";

    hFind = FindFirstFileA(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
            LARGE_INTEGER size;
            size.LowPart = fileInfo.nFileSizeLow;
            size.HighPart = fileInfo.nFileSizeHigh;
            return size.QuadPart;
        }
        cerr << "Cannot access: " << path << endl;
        return 0;
    }

    do {
        string name = fd.cFileName;
        if (name == "." || name == "..") continue;

        string fullPath = path + "\\" + name;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            totalSize += getFileOrFolderSize(fullPath);
        } else {
            LARGE_INTEGER size;
            size.LowPart = fd.nFileSizeLow;
            size.HighPart = fd.nFileSizeHigh;
            totalSize += size.QuadPart;
        }
    } while (FindNextFileA(hFind, &fd) != 0);

    FindClose(hFind);
    return totalSize;
}

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
        } 
        else if (line.substr(0, 3) == "cd ") {
            changeDirectory(line.substr(3));
        } 
        else if (line == "clear") {
            system("cls");
        } 
    }
    mch.close();
}

void runAutoexec() {
    ifstream f("autoexec.mch");
    if(f.is_open()) runMCH("autoexec.mch");
    else return;
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

        if (command.substr(0, 5) == "size ") {
            string path = command.substr(5);
            uint64_t size = getFileOrFolderSize(path);
            cout << "Size of \"" << path << "\": " << size << " bytes" << endl;
            continue;
        }

        if (command.substr(0, 5) == "find ") {
            string rest = command.substr(5);
            size_t spacePos = rest.find(' ');
            if (spacePos != string::npos) {
                string filename = rest.substr(0, spacePos);
                string search = rest.substr(spacePos + 1);
                findInFile(filename, search);
            }
            continue;
        }

        if (command.substr(0, 5) == "smch ") {
            runMCH(command.substr(5));
            continue;
        }

        if (command == "testnet") {
            testNet();
            continue;
        }

        if (command == "pwd") {
            cout << "Current path is: " << getPath() << endl; 
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