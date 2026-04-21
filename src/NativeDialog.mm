/*
 * asciiVideo — render video files as ASCII art
 * Copyright (C) 2026 Steve Meyfroidt
 *
 * Licensed under GPLv3. See LICENSE.
 */

#include "NativeDialog.h"

#import <AppKit/AppKit.h>

namespace asciiVideo {

LoadErrorChoice showConfigLoadErrorDialog(const std::string & filePath, const std::string & errorMsg) {
	@autoreleasepool {
		NSAlert * alert = [[NSAlert alloc] init];
		[alert setAlertStyle:NSAlertStyleWarning];
		[alert setMessageText:@"asciiVideo couldn't load its settings file."];

		std::string info;
		info.reserve(filePath.size() + errorMsg.size() + 16);
		info += "File:\n";
		info += filePath;
		info += "\n\n";
		info += errorMsg;
		[alert setInformativeText:[NSString stringWithUTF8String:info.c_str()]];

		[alert addButtonWithTitle:@"Continue with defaults"];
		[alert addButtonWithTitle:@"Quit"];

		NSModalResponse r = [alert runModal];
		return (r == NSAlertFirstButtonReturn)
			? LoadErrorChoice::ContinueWithDefaults
			: LoadErrorChoice::Quit;
	}
}

}
