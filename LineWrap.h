#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"

namespace UI::LineWrap {

inline std::vector<std::string> Wrap(size_t lineCountMax, size_t lineLenMax, std::string_view str) {
    using Line = std::deque<std::string>;
    using Lines = std::deque<Line>;
    
    Lines linesInput;
    {
        std::istringstream stream((std::string)str);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            
            Line& lineInput = linesInput.emplace_back();
            std::istringstream stream(line);
            std::string word;
            while (stream >> word) lineInput.push_back(word);
        }
    }
    
    std::vector<std::string> lines;
    Line* lastLine = nullptr;
    while (lines.size()<lineCountMax && !linesInput.empty()) {
        Line& lineInput = linesInput.front();
        std::string& msgline = lines.emplace_back();
        
        while (!lineInput.empty()) {
            const std::string& word = lineInput.front();
            const std::string add = (msgline.empty() ? "" : " ") + word;
            if (msgline.size()+add.size() > lineLenMax) break; // Line filled, next line
            msgline += add;
            lineInput.pop_front();
        }
        
//            if (words.empty()) break; // No more words -> done
//            if (lines.size() >= lineCountMax) break; // Hit max number of lines -> done
//            if (words.front().size() > lineLenMax) break; // Current word is too large for line -> done
        
        if (lineInput.empty()) {
            linesInput.pop_front();
            lastLine = nullptr;
        } else {
            lastLine = &linesInput.front();
        }
    }
    
    // Add as many letters from the remaining word as will fit on the last line
    if (lastLine && !lastLine->empty()) {
        const std::string& word = lastLine->front();
        std::string& line = lines.back();
        line += (line.empty() ? "" : " ") + word;
        // Our logic guarantees that if the word would have fit, it would've been included in the last line.
        // So since the word isn't included, the length of the line (with the word included) must be larger
        // than `lineLenMax`. So verify that assumption.
        assert(line.size() > lineLenMax);
        line.erase(lineLenMax);
        
//            const char*const ellipses = "...";
//            // Find the first non-space character, at least strlen(ellipses) characters before the end
//            auto it =line.begin()+lineLenMax-strlen(ellipses);
//            for (; it!=line.begin() && std::isspace(*it); it--);
//            
//            line.erase(it, line.end());
//            
//            // Replace the line's final characters with an ellipses
//            line.replace(it, it+strlen(ellipses), ellipses);
    }
    
    return lines;
}

} // namespace UI::LineWrap
