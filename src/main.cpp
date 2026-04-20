/*
 * asciiVideo — render video files as ASCII art
 * Copyright (C) 2026 Steve Meyfroidt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ofMain.h"
#include "ofApp.h"

#include <filesystem>

//========================================================================
int main( ){

	// If running from inside a .app bundle that has its own data/ folder
	// inside Contents/Resources/, prefer that over the default sibling path.
	// Lets a single-folder .app be copied anywhere and still find its data.
	{
		namespace fs = std::filesystem;
		fs::path exeDir = ofFilePath::getCurrentExeDirFS();
		fs::path bundleData = exeDir / ".." / "Resources" / "data";
		std::error_code ec;
		if (fs::is_directory(bundleData, ec)) {
			ofSetDataPathRoot(fs::canonical(bundleData, ec).string());
		}
	}

	//Use ofGLFWWindowSettings for more options like multi-monitor fullscreen
	ofGLWindowSettings settings;
	settings.setSize(1024, 768);
	settings.windowMode = OF_WINDOW; //can also be OF_FULLSCREEN

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();

}
