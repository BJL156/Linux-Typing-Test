#include <iostream>
#include <string>
#include <chrono>
#include <random>
#include <vector>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

std::random_device g_randomDevice;
std::mt19937 g_mersenneTwister(g_randomDevice());
std::chrono::time_point<std::chrono::high_resolution_clock> g_start;
std::chrono::time_point<std::chrono::high_resolution_clock> g_end;

std::string getTerminalFgColor(int r, int g, int b) {
    return "\x1B[38;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + 'm';
}

std::string getResetTerminalColor() {
    return "\x1B[0m";
}

void setTerminal(bool enable) {
    struct termios oldTermios, newTermios;
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    
    if (!enable) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
        
        return;
    }
    
    tcgetattr(STDIN_FILENO, &oldTermios);
    newTermios = oldTermios;
    
    newTermios.c_lflag &= ~(ICANON | ECHO);
    newTermios.c_cc[VMIN] = 1;
    newTermios.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
    
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void clearTerminal() {
    std::cout << "\x1B[2J\x1B[H";
}

std::vector<std::string> readFileContents(const std::string filepath) {
    std::vector<std::string> words;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "failed to open file: " << filepath << '.';
        return words;
    }
    
    std::string word;
    while (std::getline(file, word)) {
        words.push_back(word);
    }
    
    file.close();
    
    return words;
}

void handletypedString(const char& ch, std::string& typedString) {
    if (ch == 27) {
        typedString.clear();
    } else if (ch == 127) {
        if (!typedString.empty()) {
            typedString.pop_back();
        }
    } else {
        typedString.push_back(ch);
    }
}

int amountOfWords(const std::string& str) {
    int wordCount = 1;
    
    for (const char& ch : str) {
        if (ch == ' ') {
            wordCount++;
        }
    }
    
    return wordCount;
}

double getWpm(const std::string& targetString, const double& elapsedTime) {
    double wordCount = amountOfWords(targetString);
    double minutesTaken = elapsedTime / 60;
    
    double wpm = wordCount / minutesTaken;
    
    return wpm;
}

void printScreen(const std::string& typedString, const std::string& targetString) {
    std::string preMessage = "TERMINAL TYPE TEST           only 100% accuracy tests are submitted.";
    std::string endMessage = "up/down arrow - change word count          escape - restart";
    int longerMessageSize = (preMessage.size() > endMessage.size()) ? preMessage.size() : endMessage.size();
    
    std::cout << getTerminalFgColor(0, 0, 0);
    if (typedString.empty()) {
        std::cout << preMessage << "\n";
    } else {
        g_end = std::chrono::high_resolution_clock::now();
        double elapsedTime = std::chrono::duration<double>(g_end - g_start).count();
        std::cout << "time elapsed: " << elapsedTime << "\t\twpm: " << getWpm(targetString, elapsedTime) << '\n';
    }
    
    int spaceCount = (longerMessageSize - targetString.size()) / 2;
    for (int i = 0; i < spaceCount; i++) {
        std::cout << ' ';
    }
    
    for (int i = 0; i < targetString.size(); i++) {
        if (i < typedString.size()) {
            if (targetString[i] != typedString[i]) {
                std::cout << getTerminalFgColor(255, 0, 0);
            } else {
                std::cout << getTerminalFgColor(0, 0, 0);
            }
        } else {
            std::cout << getTerminalFgColor(127, 127, 127);
        }
        
        std::cout << targetString[i];
    }
    
    std::cout << '\n' << getTerminalFgColor(0, 0, 0) << endMessage;
    
    std::cout << '\n';
}

int generateRandomInt(int min, int max) {
    std::uniform_int_distribution<std::mt19937::result_type> distribution(min, max);
    return distribution(g_mersenneTwister);
}

void setupTargetString(const std::vector<std::string>& wordList, int maxWords, std::string& targetString) {
    targetString.clear();
    
    for (int i = 0; i < maxWords; i++) {
        int randomIndex = generateRandomInt(0, wordList.size() - 1);
        targetString += wordList[randomIndex] + ' ';
    }
    
    targetString.pop_back();
}

void displayScreen(const std::string& typedString, const std::string& targetString) {
    clearTerminal();
    printScreen(typedString, targetString);
}

int main() {
    setTerminal(true);
    
    const std::vector<std::string> wordList = readFileContents("wordList1000.txt");
    int maxWords = 6;
    
    std::string typedString = "";
    std::string targetString = "";
    
    g_start = std::chrono::high_resolution_clock::now();
    
    setupTargetString(wordList, maxWords, targetString);
    printScreen(typedString, targetString);
    
    while (true) {
        if (typedString.empty()) {
            g_start = std::chrono::high_resolution_clock::now();
        }
        
        char ch;
        if (read(STDIN_FILENO, &ch, 1) <= 0) {
            continue;
        }
        
        
        if (ch == 27) {
            setupTargetString(wordList, maxWords, targetString);
        } else if (ch == 65 || ch == 67) {
            maxWords++;
            
            setupTargetString(wordList, maxWords, targetString);
            typedString.clear();
            
            displayScreen(typedString, targetString);
            
            continue;
        } else if (ch == 66 || ch == 68) {
            if (maxWords > 1) {
                maxWords--;
            }
            
            setupTargetString(wordList, maxWords, targetString);
            typedString.clear();
            
            displayScreen(typedString, targetString);
            
            continue;
        }
        
        handletypedString(ch, typedString);
        
        clearTerminal();
        printScreen(typedString, targetString);
        
        if (typedString == targetString) {
            break;
        }
    }
    
    g_end = std::chrono::high_resolution_clock::now();
    double elapsedTime = std::chrono::duration<double>(g_end - g_start).count();
    
    double wpm = getWpm(targetString, elapsedTime);
    
    std::cout << "final wpm for " << maxWords << " word test: " << wpm << '\n';
    
    setTerminal(false);
    
    return 0;
}