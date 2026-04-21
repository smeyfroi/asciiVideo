/*
 * asciiVideo — render video files as ASCII art
 * Copyright (C) 2026 Steve Meyfroidt
 *
 * Licensed under GPLv3. See LICENSE.
 */

#pragma once

#include <string>

namespace asciiVideo {

enum class LoadErrorChoice { ContinueWithDefaults, Quit };

// Show a native macOS alert with "Continue with defaults" and "Quit" buttons.
// Blocks until the user dismisses it. Safe to call before the OF main loop starts.
LoadErrorChoice showConfigLoadErrorDialog(const std::string & filePath, const std::string & errorMsg);

}
