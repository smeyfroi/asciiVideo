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

#pragma once

#include "ofMain.h"
#include "ofxFFmpegRecorder.h"
#include "ofxGui.h"
#include <filesystem>

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

private:
	struct PaletteEntry {
		char ch;
		ofColor color;
	};
	struct Palette {
		std::string name;
		ofColor backgroundColor { 0, 0, 0 };
		std::vector<PaletteEntry> entries;
	};

	void resolveConfigDir();
	void loadConfig();
	void installFallbackPalette();
	void setupGui();
	void reloadFont();
	void onPaletteIndexChanged(int & idx);
	void onFontSizeChanged(int & v);
	void onFontPathChanged(std::string & v);
	void onPickFontPressed();
	void onSamplePalettePressed();
	void saveSettings();

	bool beginSampling(const std::filesystem::path & videoPath);
	void tickSampling();
	void finishSampling();
	void cancelSampling();
	std::vector<ofColor> kmeansColours(const std::vector<ofColor> & samples, int k);
	void appendPaletteToConfigJson(const Palette & pal);

	void startProcessing(const std::filesystem::path & path);
	void finishProcessing();

	ofVideoPlayer player;
	ofFbo renderFbo;
	ofPixels frameBuf;
	ofxFFmpegRecorder recorder;
	ofTrueTypeFont font;

	std::vector<Palette> palettes;
	ofJson configJson;
	bool configLoaded = false;
	std::filesystem::path configDir;
	std::filesystem::path configJsonPath;
	std::filesystem::path inputPath;
	std::filesystem::path outputPath;

	bool processing = false;
	int framesWritten = 0;

	// Sampling state machine
	ofVideoPlayer samplerPlayer;
	bool isSampling = false;
	int samplingBaseIndex = 0;
	int samplingTargetCount = 0;
	int samplingTotalFrames = 0;
	int samplingCollected = 0;
	int samplingAttempted = 0;
	int samplingPixelStride = 8;
	bool samplingSeekIssued = false;
	int samplingLastObservedFrame = -1;
	uint64_t samplingStartMs = 0;
	uint64_t samplingSeekIssuedMs = 0;
	std::vector<ofColor> samplingBuffer;
	std::string samplingSourceName;

	ofxPanel gui;
	ofxButton pickFontButton;
	ofxButton samplePaletteButton;
	ofxLabel fontDisplay;
	ofParameter<int> paletteIndex;
	ofParameter<std::string> paletteLabel;
	ofParameter<std::string> fontPath;
	ofParameter<int> fontSize;
	ofParameter<float> gamma;
	ofParameter<int> cellW;
	ofParameter<int> cellH;
};
