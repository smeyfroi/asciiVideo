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

#include "ofApp.h"
#include "NativeDialog.h"

#include <cmath>
#include <fstream>

namespace {
constexpr const char * kConfigFileName = "asciiVideo-settings.json";
constexpr const char * kAppName = "asciiVideo";
constexpr const char * kPointerFileName = "config-path.txt";
}

//--------------------------------------------------------------
void ofApp::setup() {
	ofBackground(0);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();

	resolveConfigDir();
	loadConfig();
	setupGui();
	reloadFont();

	ofSetWindowTitle("asciiVideo — drag a video file onto this window");
}

//--------------------------------------------------------------
void ofApp::exit() {
	saveSettings();
}

//--------------------------------------------------------------
void ofApp::resolveConfigDir() {
	namespace fs = std::filesystem;

	fs::path appSupportDir = fs::path(ofFilePath::getUserHomeDir()) / "Library" / "Application Support" / kAppName;
	std::error_code ec;
	fs::create_directories(appSupportDir, ec);
	fs::path pointerPath = appSupportDir / kPointerFileName;

	fs::path chosen;
	if (fs::exists(pointerPath, ec)) {
		std::ifstream in(pointerPath);
		std::string stored;
		if (in) std::getline(in, stored);
		while (!stored.empty() && (stored.back() == '\n' || stored.back() == '\r' || stored.back() == ' ')) {
			stored.pop_back();
		}
		if (!stored.empty() && fs::is_directory(stored, ec)) {
			chosen = stored;
		} else if (!stored.empty()) {
			ofLogWarning() << "stored config path is no longer a directory, re-prompting: " << stored;
		}
	}

	if (chosen.empty()) {
		ofFileDialogResult dlg = ofSystemLoadDialog(
			"Choose a folder for asciiVideo palettes and settings", true,
			ofFilePath::getUserHomeDir());
		if (dlg.bSuccess && !dlg.getPath().empty() && fs::is_directory(dlg.getPath(), ec)) {
			chosen = dlg.getPath();
			std::ofstream out(pointerPath, std::ios::trunc);
			if (out) {
				out << chosen.string();
				ofLogNotice() << "saved config location pointer -> " << chosen;
			} else {
				ofLogError() << "failed to write pointer file at " << pointerPath;
			}
		} else {
			ofLogWarning() << "no config folder chosen; falling back to " << appSupportDir;
			chosen = appSupportDir;
		}
	}

	configDir = chosen;
	configJsonPath = configDir / kConfigFileName;

	if (!fs::exists(configJsonPath, ec)) {
		fs::path bundled = ofToDataPath(kConfigFileName, true);
		if (fs::exists(bundled, ec) && fs::absolute(bundled, ec) != fs::absolute(configJsonPath, ec)) {
			fs::copy_file(bundled, configJsonPath, fs::copy_options::none, ec);
			if (ec) {
				ofLogError() << "failed to seed palettes.json into " << configDir << ": " << ec.message();
			} else {
				ofLogNotice() << "seeded palettes.json into " << configDir;
			}
		}
	}

	ofLogNotice() << "config dir: " << configDir;
}

//--------------------------------------------------------------
void ofApp::loadConfig() {
	palettes.clear();

	std::string fontPathDefault = "Courier New Bold.ttf";
	int fontSizeDefault = 9;
	float gammaDefault = 1.0f;
	int cellWDefault = 7;
	int cellHDefault = 9;
	std::string startPalette;

	configLoaded = false;
	configJson = ofJson{};

	if (!std::filesystem::exists(configJsonPath)) {
		ofLogWarning() << "no " << configJsonPath << "; using built-in fallback palette";
		installFallbackPalette();
	} else {
		std::ifstream ifs(configJsonPath);
		if (!ifs) {
			ofLogError() << "cannot open " << configJsonPath;
			installFallbackPalette();
		} else try {
			configJson = ofJson::parse(ifs);
			configLoaded = true;

			if (configJson.contains("settings")) {
				const auto & d = configJson["settings"];
				if (d.contains("fontPath")) fontPathDefault = d["fontPath"].get<std::string>();
				if (d.contains("fontSize")) fontSizeDefault = d["fontSize"].get<int>();
				if (d.contains("gamma")) gammaDefault = d["gamma"].get<float>();
				if (d.contains("cellWidth")) cellWDefault = d["cellWidth"].get<int>();
				if (d.contains("cellHeight")) cellHDefault = d["cellHeight"].get<int>();
				if (d.contains("startPalette")) startPalette = d["startPalette"].get<std::string>();
			}

			if (configJson.contains("palettes")) {
				for (const auto & p : configJson["palettes"]) {
					Palette pal;
					pal.name = p.value("name", std::string("unnamed"));
					if (p.contains("backgroundColor")) {
						const auto & bg = p["backgroundColor"];
						pal.backgroundColor.set(
							bg.size() > 0 ? bg[0].get<int>() : 0,
							bg.size() > 1 ? bg[1].get<int>() : 0,
							bg.size() > 2 ? bg[2].get<int>() : 0);
					}
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
			ofLogError() << "failed to parse " << configJsonPath << ": " << e.what();
			configLoaded = false;
			auto choice = asciiVideo::showConfigLoadErrorDialog(configJsonPath.string(), e.what());
			if (choice == asciiVideo::LoadErrorChoice::Quit) {
				std::exit(0);
			}
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
	gamma.set("gamma", gammaDefault, 0.1f, 4.0f);
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
	gui.setup(kAppName);
	gui.disableHeader();
	gui.add(paletteIndex);
	gui.add(paletteLabel);

	pickFontButton.setup("pick font\u2026");
	pickFontButton.addListener(this, &ofApp::onPickFontPressed);
	gui.add(&pickFontButton);

	fontDisplay.setup("font", std::filesystem::path(fontPath.get()).filename().string());
	gui.add(&fontDisplay);

	gui.add(fontSize);
	gui.add(gamma);
	gui.add(cellW);
	gui.add(cellH);

	samplePaletteButton.setup("sample palette from video\u2026");
	samplePaletteButton.addListener(this, &ofApp::onSamplePalettePressed);
	gui.add(&samplePaletteButton);

	paletteIndex.addListener(this, &ofApp::onPaletteIndexChanged);
	fontSize.addListener(this, &ofApp::onFontSizeChanged);
	fontPath.addListener(this, &ofApp::onFontPathChanged);

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
	fontDisplay = std::filesystem::path(v).filename().string();
}

//--------------------------------------------------------------
void ofApp::onPickFontPressed() {
	ofFileDialogResult r = ofSystemLoadDialog("Choose a font file (.ttf, .otf)", false);
	if (r.bSuccess && !r.getPath().empty()) {
		fontPath = r.getPath();
	}
}

//--------------------------------------------------------------
void ofApp::onSamplePalettePressed() {
	if (isSampling) {
		ofLogWarning() << "already sampling; ignoring press";
		return;
	}
	if (processing) {
		ofLogWarning() << "cannot sample while rendering; ignoring press";
		return;
	}
	ofFileDialogResult r = ofSystemLoadDialog("Choose a video to sample colours from", false);
	if (!r.bSuccess || r.getPath().empty()) return;

	if (!beginSampling(r.getPath())) {
		ofLogError() << "failed to start sampling from " << r.getPath();
	}
}

//--------------------------------------------------------------
bool ofApp::beginSampling(const std::filesystem::path & videoPath) {
	if (palettes.empty()) {
		ofLogError() << "no base palette to derive from";
		return false;
	}
	if (!samplerPlayer.load(videoPath.string())) return false;

	samplingTotalFrames = samplerPlayer.getTotalNumFrames();
	if (samplingTotalFrames <= 0) {
		samplerPlayer.close();
		return false;
	}

	samplingBaseIndex = ofClamp(paletteIndex.get(), 0, (int)palettes.size() - 1);
	samplingTargetCount = std::min(samplingTotalFrames, 60);
	samplingCollected = 0;
	samplingAttempted = 0;
	samplingBuffer.clear();
	samplingBuffer.reserve(samplingTargetCount * 2000);
	samplingSeekIssued = false;
	samplingLastObservedFrame = -1;
	samplingStartMs = ofGetElapsedTimeMillis();
	samplingSourceName = videoPath.stem().string();

	samplerPlayer.setLoopState(OF_LOOP_NONE);
	samplerPlayer.play();
	isSampling = true;

	ofLogNotice() << "sampling " << samplingTargetCount << " frames from " << videoPath;
	return true;
}

//--------------------------------------------------------------
void ofApp::tickSampling() {
	if (!isSampling) return;

	samplerPlayer.update();

	const uint64_t now = ofGetElapsedTimeMillis();
	const uint64_t totalElapsed = now - samplingStartMs;
	const uint64_t kOverallCapMs = 30000;
	const uint64_t kSeekTimeoutMs = 1000;

	if (totalElapsed > kOverallCapMs) {
		ofLogWarning() << "sampling hit overall cap (" << kOverallCapMs << "ms); finishing with " << samplingCollected << "/" << samplingTargetCount;
		finishSampling();
		return;
	}

	if (samplingAttempted >= samplingTargetCount) {
		finishSampling();
		return;
	}

	if (!samplingSeekIssued) {
		const int target = (int)((samplingAttempted + 0.5f) * samplingTotalFrames / samplingTargetCount);
		samplerPlayer.setFrame(target);
		samplingSeekIssued = true;
		samplingSeekIssuedMs = now;
		samplingLastObservedFrame = samplerPlayer.getCurrentFrame();
		return;
	}

	const int cur = samplerPlayer.getCurrentFrame();
	const bool frameLanded = samplerPlayer.isFrameNew() && cur != samplingLastObservedFrame;

	if (frameLanded) {
		const ofPixels & px = samplerPlayer.getPixels();
		if (px.isAllocated()) {
			const int w = px.getWidth();
			const int h = px.getHeight();
			const int stride = std::max(1, samplingPixelStride);
			for (int y = 0; y < h; y += stride) {
				for (int x = 0; x < w; x += stride) {
					samplingBuffer.push_back(px.getColor(x, y));
				}
			}
			++samplingCollected;
		}
		++samplingAttempted;
		samplingSeekIssued = false;
		return;
	}

	if (now - samplingSeekIssuedMs > kSeekTimeoutMs) {
		ofLogWarning() << "seek timeout for sample " << (samplingAttempted + 1) << "/" << samplingTargetCount << " — skipping";
		++samplingAttempted;
		samplingSeekIssued = false;
	}
}

//--------------------------------------------------------------
void ofApp::cancelSampling() {
	if (!isSampling) return;
	samplerPlayer.stop();
	samplerPlayer.close();
	samplingBuffer.clear();
	isSampling = false;
	ofLogNotice() << "sampling cancelled";
}

//--------------------------------------------------------------
void ofApp::finishSampling() {
	if (!isSampling) return;
	samplerPlayer.stop();
	samplerPlayer.close();
	isSampling = false;

	if (samplingBuffer.empty() || samplingBaseIndex < 0 || samplingBaseIndex >= (int)palettes.size()) {
		ofLogError() << "sampling produced no usable pixels";
		return;
	}

	const Palette & base = palettes[samplingBaseIndex];
	const int k = (int)base.entries.size();

	ofLogNotice() << "running k-means on " << samplingBuffer.size() << " samples (k=" << k << ")";
	auto centers = kmeansColours(samplingBuffer, k);
	samplingBuffer.clear();
	samplingBuffer.shrink_to_fit();

	if ((int)centers.size() != k) {
		ofLogError() << "k-means returned " << centers.size() << " centers, expected " << k;
		return;
	}

	std::sort(centers.begin(), centers.end(), [](const ofColor & a, const ofColor & b) {
		return a.getLightness() < b.getLightness();
	});

	Palette out;
	out.name = base.name + " + " + samplingSourceName;
	out.backgroundColor = base.backgroundColor;
	out.entries.reserve(k);
	for (int i = 0; i < k; ++i) {
		PaletteEntry e;
		e.ch = base.entries[i].ch;
		e.color = centers[i];
		out.entries.push_back(e);
	}

	palettes.push_back(std::move(out));
	appendPaletteToConfigJson(palettes.back());

	int newIdx = (int)palettes.size() - 1;
	paletteIndex.set("paletteIndex", newIdx, 0, newIdx);
	paletteLabel = palettes[newIdx].name;

	saveSettings();

	ofLogNotice() << "added palette: " << palettes[newIdx].name << " (" << samplingCollected << " frames sampled)";
}

//--------------------------------------------------------------
std::vector<ofColor> ofApp::kmeansColours(const std::vector<ofColor> & samples, int k) {
	if (samples.empty() || k <= 0) return {};

	std::vector<ofColor> centers;
	centers.reserve(k);
	centers.push_back(samples[(size_t)ofRandom(samples.size())]);

	// k-means++ init
	while ((int)centers.size() < k) {
		std::vector<float> distSq(samples.size());
		float total = 0.0f;
		for (size_t i = 0; i < samples.size(); ++i) {
			float minD = std::numeric_limits<float>::max();
			for (const auto & c : centers) {
				float dr = float(samples[i].r) - float(c.r);
				float dg = float(samples[i].g) - float(c.g);
				float db = float(samples[i].b) - float(c.b);
				float d = dr * dr + dg * dg + db * db;
				if (d < minD) minD = d;
			}
			distSq[i] = minD;
			total += minD;
		}
		if (total <= 0.0f) {
			centers.push_back(samples[(size_t)ofRandom(samples.size())]);
			continue;
		}
		float pick = ofRandom(total);
		float cumul = 0.0f;
		size_t chosen = samples.size() - 1;
		for (size_t i = 0; i < samples.size(); ++i) {
			cumul += distSq[i];
			if (cumul >= pick) { chosen = i; break; }
		}
		centers.push_back(samples[chosen]);
	}

	// Lloyd's iterations
	std::vector<int> assign(samples.size(), -1);
	const int maxIter = 20;
	for (int iter = 0; iter < maxIter; ++iter) {
		bool changed = false;
		for (size_t i = 0; i < samples.size(); ++i) {
			float minD = std::numeric_limits<float>::max();
			int best = 0;
			for (int ci = 0; ci < k; ++ci) {
				float dr = float(samples[i].r) - float(centers[ci].r);
				float dg = float(samples[i].g) - float(centers[ci].g);
				float db = float(samples[i].b) - float(centers[ci].b);
				float d = dr * dr + dg * dg + db * db;
				if (d < minD) { minD = d; best = ci; }
			}
			if (assign[i] != best) { assign[i] = best; changed = true; }
		}
		if (!changed) break;

		std::vector<double> sumR(k, 0), sumG(k, 0), sumB(k, 0);
		std::vector<int> counts(k, 0);
		for (size_t i = 0; i < samples.size(); ++i) {
			int ci = assign[i];
			sumR[ci] += samples[i].r;
			sumG[ci] += samples[i].g;
			sumB[ci] += samples[i].b;
			counts[ci]++;
		}
		for (int ci = 0; ci < k; ++ci) {
			if (counts[ci] > 0) {
				centers[ci].r = (int)std::round(sumR[ci] / counts[ci]);
				centers[ci].g = (int)std::round(sumG[ci] / counts[ci]);
				centers[ci].b = (int)std::round(sumB[ci] / counts[ci]);
			}
		}
	}

	return centers;
}

//--------------------------------------------------------------
void ofApp::appendPaletteToConfigJson(const Palette & pal) {
	if (!configLoaded) return;
	ofJson pj;
	pj["name"] = pal.name;
	pj["backgroundColor"] = { (int)pal.backgroundColor.r, (int)pal.backgroundColor.g, (int)pal.backgroundColor.b };
	pj["characters"] = ofJson::array();
	for (const auto & e : pal.entries) {
		ofJson ej;
		ej["char"] = std::string(1, e.ch);
		ej["color"] = { (int)e.color.r, (int)e.color.g, (int)e.color.b };
		pj["characters"].push_back(ej);
	}
	if (!configJson.contains("palettes")) {
		configJson["palettes"] = ofJson::array();
	}
	configJson["palettes"].push_back(pj);
}

//--------------------------------------------------------------
void ofApp::saveSettings() {
	if (!configLoaded || configJsonPath.empty()) return;

	configJson["settings"]["fontPath"] = fontPath.get();
	configJson["settings"]["fontSize"] = fontSize.get();
	configJson["settings"]["gamma"] = gamma.get();
	configJson["settings"]["cellWidth"] = cellW.get();
	configJson["settings"]["cellHeight"] = cellH.get();
	if (!palettes.empty()) {
		int idx = ofClamp(paletteIndex.get(), 0, (int)palettes.size() - 1);
		configJson["settings"]["startPalette"] = palettes[idx].name;
	}

	if (!ofSavePrettyJson(configJsonPath.string(), configJson)) {
		ofLogError() << "failed to save settings to " << configJsonPath;
	}
}

//--------------------------------------------------------------
void ofApp::update() {
	if (isSampling) {
		tickSampling();
		return;
	}
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
		ofClear(pal.backgroundColor.r, pal.backgroundColor.g, pal.backgroundColor.b, 255);
		for (int i = 0; i < w; i += stepX) {
			for (int j = 0; j < h; j += stepY) {
				const int xEnd = std::min(i + stepX, w);
				const int yEnd = std::min(j + stepY, h);
				long rSum = 0, gSum = 0, bSum = 0;
				int count = 0;
				for (int yy = j; yy < yEnd; ++yy) {
					for (int xx = i; xx < xEnd; ++xx) {
						ofColor c = src.getColor(xx, yy);
						rSum += c.r;
						gSum += c.g;
						bSum += c.b;
						++count;
					}
				}
				ofColor avg(rSum / count, gSum / count, bSum / count);
				float lightness = avg.getLightness();
				float normalized = ofMap(lightness, 0, 255, 0, 1, true);
				int idx = std::pow(normalized, gamma.get()) * last;
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

	if (isSampling) {
		const int margin = 40;
		const int barH = 24;
		const int barW = ofGetWidth() - margin * 2;
		const int barX = margin;
		const int barY = ofGetHeight() / 2 - barH / 2;
		float progress = samplingTargetCount > 0 ? (float)samplingAttempted / (float)samplingTargetCount : 0.0f;

		ofPushStyle();
		ofSetColor(0, 0, 0, 200);
		ofDrawRectangle(0, barY - 60, ofGetWidth(), 140);
		ofSetColor(255);
		ofNoFill();
		ofDrawRectangle(barX, barY, barW, barH);
		ofFill();
		ofSetColor(120, 200, 255);
		ofDrawRectangle(barX, barY, barW * ofClamp(progress, 0.0f, 1.0f), barH);
		ofSetColor(255);
		std::string label = "sampling palette from " + samplingSourceName + "\n"
			+ ofToString(samplingAttempted) + " / " + ofToString(samplingTargetCount) + " frames"
			+ "   (" + ofToString((int)(progress * 100)) + "%)";
		ofDrawBitmapStringHighlight(label, barX, barY - 20);
		ofDrawBitmapString("press esc to cancel", barX, barY + barH + 20);
		ofPopStyle();
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

	renderFbo.allocate(w, h, GL_RGB, 4);
	ofSetWindowShape(w, h);

	float fps = 30.0f;
	if (player.getTotalNumFrames() > 0 && player.getDuration() > 0.0f) {
		fps = player.getTotalNumFrames() / player.getDuration();
	}

	recorder.setup(true, false, glm::vec2(w, h), fps);
	recorder.setOverWrite(true);
	recorder.setFFmpegPath(ofToDataPath("ffmpeg", true));
	recorder.setInputPixelFormat(OF_IMAGE_COLOR);
	recorder.setVideoCodec("libx264");
	recorder.addAdditionalOutputArgument("-crf 18");
	recorder.addAdditionalOutputArgument("-preset medium");
	recorder.addAdditionalOutputArgument("-pix_fmt yuv420p");
	recorder.addAdditionalOutputArgument("-vf 'scale=trunc(iw/2)*2:trunc(ih/2)*2'");
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
	if (isSampling) {
		ofLogWarning() << "sampling in progress; drop ignored";
		return;
	}
	if (dragInfo.files.empty()) return;
	startProcessing(std::filesystem::path(dragInfo.files.front()));
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == 'q' && processing) {
		ofLogNotice() << "user requested early stop";
		finishProcessing();
	}
	if (key == OF_KEY_ESC && isSampling) {
		cancelSampling();
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
