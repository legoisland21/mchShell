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
    vector<string> sites = { "google.com", "yahoo.com", "example.com", "wikipedia.org", "bing.com" };

    for (auto &site : sites) {
        string cmd = "ping -n 1 " + site + " >nul";
        int res = system(cmd.c_str());
        SetConsoleTextAttribute(hConsole, res == 0 ? 10 : 12);
        cout << site << (res == 0 ? " is reachable" : " is unreachable") << endl;
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

ULONGLONG getFileOrFolderSize(const string &path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fad)) {
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ULONGLONG size = 0;
            WIN32_FIND_DATAA fd;
            HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &fd);
            if (hFind == INVALID_HANDLE_VALUE) return 0;
            do {
                string name = fd.cFileName;
                if (name != "." && name != "..")
                    size += getFileOrFolderSize(path + "\\" + name);
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
            return size;
        } else
            return ((ULONGLONG)fad.nFileSizeHigh << 32) + fad.nFileSizeLow;
    }
    return 0;
}

void findFilesAndFolders(const string& term) {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("*", &fd);
    if (h == INVALID_HANDLE_VALUE) { cout << "Nothing found\n"; return; }

    string files, folders;
    do {
        string n = fd.cFileName;
        if (n == "." || n == "..") continue;
        if (n.substr(0, term.size()) == term) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                folders += n + " ";
            else
                files += n + " ";
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);

    if (!files.empty()) cout << "FILES\n" << files << "\n";
    if (!folders.empty()) cout << "FOLDERS\n" << folders << "\n";
    if (files.empty() && folders.empty()) cout << "No files/folders found\n";
}

// Environment Variable handler

string getEnvironmentVariable(const string& name) {
    char buf[1024];
    DWORD sz = GetEnvironmentVariableA(name.c_str(), buf, sizeof(buf));
    if (sz == 0) return "";
    if (sz > sizeof(buf)) { vector<char> b(sz); GetEnvironmentVariableA(name.c_str(), b.data(), sz); return string(b.data()); }
    return string(buf);
}

string expandEnvironmentVariables(const string& input) {
    string r = input; size_t pos = 0;
    while ((pos = r.find('%', pos)) != string::npos) {
        size_t end = r.find('%', pos+1);
        if (end == string::npos) break;
        string val = getEnvironmentVariable(r.substr(pos+1, end-pos-1));
        r.replace(pos, end-pos+1, val); pos += val.size();
    }
    return r;
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

        if (command.substr(0, 5) == "smch ") { runMCH(command.substr(5)); continue; }

        if (command == "testnet") { testNet(); continue; }

        if (command == "pwd") { cout << "Current path is: " << getPath() << endl; continue; }

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