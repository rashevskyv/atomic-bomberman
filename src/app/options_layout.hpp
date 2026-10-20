// Where the rows of the Options screen are drawn, in screen units (640 x 480). The drawing,
// the cursor and the tests read these numbers, so they cannot drift apart.
#pragma once

namespace ab::options {

constexpr int kRows = 20;         // the rows, in order (main.cpp): the last one is the language
constexpr int kLanguageRow = 19;  // Language: English, Українська, ... (Left and Right choose)
constexpr float kTextX = 55.0f;   // the row text's left edge; the cursor is left of it
constexpr float kRowTop = 30.0f;  // the first row's text top (40 in the original, which had 18 rows)
constexpr float kRowStep = 21.0f; // the space between rows (22 in the original): 20 rows fit above the hint
constexpr float kHintY = 456.0f;  // the key hint under the rows (it was 440 with 18 rows)
constexpr float kTextRight = 635.0f;  // a row must end before this x (the screen edge less a margin)

// The top of row r's text.
constexpr float rowY(int r) { return kRowTop + kRowStep * static_cast<float>(r); }

}  // namespace ab::options
