// Where the rows of the Options screen are drawn, in screen units (640 x 480). The drawing,
// the cursor and the tests read these numbers, so they cannot drift apart.
#pragma once

namespace ab::options {

constexpr int kRows = 19;         // the rows, in order (main.cpp): the last one is the language
constexpr int kLanguageRow = 18;  // Language: English, Українська, ... (Left and Right choose)
constexpr float kTextX = 55.0f;   // the row text's left edge; the cursor is left of it
constexpr float kRowTop = 40.0f;  // the first row's text top
constexpr float kRowStep = 22.0f; // the original's spacing between rows
constexpr float kHintY = 456.0f;  // the key hint under the rows (it was 440 with 18 rows)
constexpr float kTextRight = 635.0f;  // a row must end before this x (the screen edge less a margin)

// The top of row r's text.
constexpr float rowY(int r) { return kRowTop + kRowStep * static_cast<float>(r); }

}  // namespace ab::options
