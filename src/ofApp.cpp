#include "ofApp.h"

namespace {
constexpr const char * kConfigFile = "palettes.json";
constexpr const char * kSettingsFile = "settings.xml";
}

//--------------------------------------------------------------
void ofApp::setup() {
	ofBackground(0);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();

	loadConfig();
	setupGui();
	reloadFont();

	ofSetWindowTitle("asciiVideo — drag a video file onto this window");
}

//--------------------------------------------------------------
void ofApp::exit() {
	gui.saveToFile(kSettingsFile);
}

//--------------------------------------------------------------
void ofApp::loadConfig() {
	palettes.clear();

	std::string fontPathDefault = "Courier New Bold.ttf";
	int fontSizeDefault = 9;
	int cellWDefault = 7;
	int cellHDefault = 9;
	std::string startPalette;

	ofFile cfgFile(kConfigFile);
	if (!cfgFile.exists()) {
		ofLogWarning() << "no " << kConfigFile << " in data folder; using built-in fallback palette";
		installFallbackPalette();
	} else {
		try {
			ofJson cfg = ofLoadJson(kConfigFile);

			if (cfg.contains("defaults")) {
				const auto & d = cfg["defaults"];
				if (d.contains("fontPath")) fontPathDefault = d["fontPath"].get<std::string>();
				if (d.contains("fontSize")) fontSizeDefault = d["fontSize"].get<int>();
				if (d.contains("cellWidth")) cellWDefault = d["cellWidth"].get<int>();
				if (d.contains("cellHeight")) cellHDefault = d["cellHeight"].get<int>();
				if (d.contains("startPalette")) startPalette = d["startPalette"].get<std::string>();
			}

			if (cfg.contains("palettes")) {
				for (const auto & p : cfg["palettes"]) {
					Palette pal;
					pal.name = p.value("name", std::string("unnamed"));
					for (const auto & e : p["characters"]) {
						PaletteEntry pe;
						std::string s = e.value("char", std::string(" "));
						pe.ch = s.empty() ? ' ' : s[0];
						auto col = e["color"];
						pe.color.set(
							col.size() > 0 ? col[0].get<int>() : 255,
							col.size() > 1 ? col[1].get<int>() : 255,
							col.size() > 2 ? col[2].get<int>() : 255);
						pal.entries.push_back(pe);
					}
					if (!pal.entries.empty()) palettes.push_back(std::move(pal));
				}
			}
		} catch (const std::exception & e) {
			ofLogError() << "failed to parse " << kConfigFile << ": " << e.what();
		}

		if (palettes.empty()) {
			ofLogWarning() << "no valid palettes in config; using built-in fallback";
			installFallbackPalette();
		}
	}

	int startIdx = 0;
	if (!startPalette.empty()) {
		for (size_t i = 0; i < palettes.size(); ++i) {
			if (palettes[i].name == startPalette) {
				startIdx = (int)i;
				break;
			}
		}
	}

	fontPath.set("fontPath", fontPathDefault);
	fontSize.set("fontSize", fontSizeDefault, 4, 48);
	cellW.set("cellWidth", cellWDefault, 2, 64);
	cellH.set("cellHeight", cellHDefault, 2, 64);
	paletteIndex.set("paletteIndex", startIdx, 0, std::max(0, (int)palettes.size() - 1));
	paletteLabel.set("palette", palettes[startIdx].name);
}

//--------------------------------------------------------------
void ofApp::installFallbackPalette() {
	Palette pal;
	pal.name = "fallback";
	const std::string chars = " .:-+*%@";
	for (size_t i = 0; i < chars.size(); ++i) {
		PaletteEntry e;
		e.ch = chars[i];
		int v = (int)ofMap(i, 0, chars.size() - 1, 0, 255);
		e.color.set(v, v, v);
		pal.entries.push_back(e);
	}
	palettes.push_back(std::move(pal));
}

//--------------------------------------------------------------
void ofApp::setupGui() {
	gui.setup("asciiVideo", kSettingsFile);
	gui.add(paletteIndex);
	gui.add(paletteLabel);
	gui.add(fontPath);
	gui.add(fontSize);
	gui.add(cellW);
	gui.add(cellH);

	paletteIndex.addListener(this, &ofApp::onPaletteIndexChanged);
	fontSize.addListener(this, &ofApp::onFontSizeChanged);
	fontPath.addListener(this, &ofApp::onFontPathChanged);

	gui.loadFromFile(kSettingsFile);

	int loadedIdx = ofClamp(paletteIndex.get(), 0, (int)palettes.size() - 1);
	paletteIndex = loadedIdx;
	paletteLabel = palettes[loadedIdx].name;
}

//--------------------------------------------------------------
void ofApp::reloadFont() {
	if (!font.load(fontPath.get(), fontSize.get())) {
		ofLogError() << "failed to load font: " << fontPath.get() << " @ " << fontSize.get();
	}
}

//--------------------------------------------------------------
void ofApp::onPaletteIndexChanged(int & idx) {
	if (palettes.empty()) return;
	int clamped = ofClamp(idx, 0, (int)palettes.size() - 1);
	paletteLabel = palettes[clamped].name;
}

//--------------------------------------------------------------
void ofApp::onFontSizeChanged(int & v) {
	reloadFont();
}

//--------------------------------------------------------------
void ofApp::onFontPathChanged(std::string & v) {
	reloadFont();
}

//--------------------------------------------------------------
void ofApp::update() {
	if (!processing) return;

	player.update();

	if (player.getIsMovieDone()) {
		finishProcessing();
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	if (processing && player.isFrameNew()) {
		const ofPixels & src = player.getPixels();
		const int w = src.getWidth();
		const int h = src.getHeight();
		const int stepX = std::max(1, cellW.get());
		const int stepY = std::max(1, cellH.get());

		const Palette & pal = palettes[ofClamp(paletteIndex.get(), 0, (int)palettes.size() - 1)];
		const int last = (int)pal.entries.size() - 1;

		renderFbo.begin();
		ofClear(0, 0, 0, 255);
		for (int i = 0; i < w; i += stepX) {
			for (int j = 0; j < h; j += stepY) {
				float lightness = src.getColor(i, j).getLightness();
				int idx = powf(ofMap(lightness, 0, 255, 0, 1), 2.5f) * last;
				idx = ofClamp(idx, 0, last);
				const PaletteEntry & e = pal.entries[idx];
				ofSetColor(e.color);
				font.drawString(std::string(1, e.ch), i, j);
			}
		}
		renderFbo.end();

		renderFbo.readToPixels(frameBuf);
		if (frameBuf.getWidth() > 0 && frameBuf.getHeight() > 0) {
			recorder.addFrame(frameBuf);
			framesWritten++;
		}
	}

	ofSetColor(255);
	if (renderFbo.isAllocated()) {
		renderFbo.draw(0, 0);
	} else {
		ofDrawBitmapString("Drag a video file onto this window to render it as ASCII.\nOutput will be written next to the source as <name>_ascii.mp4.",
			20, 40);
	}

	if (processing) {
		std::string hud = "rec  " + ofToString(framesWritten) + " / " + ofToString(player.getTotalNumFrames()) + " frames";
		ofDrawBitmapStringHighlight(hud, 10, ofGetHeight() - 20);
	}

	gui.draw();
}

//--------------------------------------------------------------
void ofApp::startProcessing(const std::filesystem::path & path) {
	if (processing) {
		ofLogWarning() << "already processing, ignoring drop: " << path;
		return;
	}

	inputPath = path;
	if (!player.load(inputPath.string())) {
		ofLogError() << "failed to load video: " << inputPath;
		return;
	}

	const int w = player.getWidth();
	const int h = player.getHeight();
	if (w <= 0 || h <= 0) {
		ofLogError() << "video reports zero dimensions: " << inputPath;
		return;
	}

	outputPath = inputPath.parent_path() / (inputPath.stem().string() + "_ascii.mp4");

	renderFbo.allocate(w, h, GL_RGB);
	ofSetWindowShape(w, h);

	float fps = 30.0f;
	if (player.getTotalNumFrames() > 0 && player.getDuration() > 0.0f) {
		fps = player.getTotalNumFrames() / player.getDuration();
	}

	recorder.setup(true, false, glm::vec2(w, h), fps);
	recorder.setOverWrite(true);
	recorder.setFFmpegPathToAddonsPath();
	recorder.setInputPixelFormat(OF_IMAGE_COLOR);
	recorder.setOutputPath(outputPath.string());
	recorder.startCustomRecord();

	framesWritten = 0;
	player.play();
	processing = true;

	ofLogNotice() << "processing " << inputPath << " (" << w << "x" << h << " @ " << fps << " fps) -> " << outputPath;
}

//--------------------------------------------------------------
void ofApp::finishProcessing() {
	if (!processing) return;
	recorder.stop();
	player.stop();
	player.close();
	processing = false;
	ofLogNotice() << "wrote " << framesWritten << " frames to " << outputPath;
	ofSetWindowTitle("asciiVideo — done. Drag another video to process.");
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (dragInfo.files.empty()) return;
	startProcessing(std::filesystem::path(dragInfo.files.front()));
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == 'q' && processing) {
		ofLogNotice() << "user requested early stop";
		finishProcessing();
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}
void ofApp::mouseDragged(int x, int y, int button) {}
void ofApp::mousePressed(int x, int y, int button) {}
void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::gotMessage(ofMessage msg) {}
