#include "SDL.h"
#include "Application.h"
#include "Vec.h"
#include <memory>
#include <vector>
#include <deque>
#include <stack>
#include <cmath>

Colour operator+(const Colour& lhs, const Colour& rhs) {
	return Colour {
			(uint8_t)(lhs.r + rhs.r),
			(uint8_t)(lhs.g + rhs.g),
			(uint8_t)(lhs.b + rhs.b),
		0xFF
	};
}

Colour operator-(const Colour& lhs, const Colour& rhs) {
	return Colour {
			(uint8_t)(lhs.r - rhs.r),
			(uint8_t)(lhs.g - rhs.g),
			(uint8_t)(lhs.b - rhs.b),
			0xFF
	};
}

Colour& operator+=(Colour& lhs, const Colour& rhs) {
	lhs = lhs + rhs;
	return lhs;
}

Colour& operator-=(Colour& lhs, const Colour& rhs) {
    lhs = lhs - rhs;
    return lhs;
}

struct App : public Application {

	bool OnStart() override {
		SetFullscreenState(true);

		screenSize = Veci{ GetClientWidth(), GetClientHeight() };
		canvasTex = std::make_unique<Texture>(*this, screenSize.x, screenSize.y);
		canvas.resize(screenSize.x * screenSize.y, Colour::Black());
		return true;
	}

	bool OnUpdate() override {

		// Check if screen size changed
		Veci newScreenSize = Veci{ GetClientWidth(), GetClientHeight() };
		if (newScreenSize != screenSize) {
			screenSize = newScreenSize;
			OnScreenSizeChanged();
		}

		// Handle drawing
		auto cursorPositions = GetCursorPositions();
		if (cursorPositions.empty()) {
		    if (!undoHistory.empty() && GetKeyUp(Key::Mouse())) {
		        pen.pressure = lastTouchPos.pressure;
				OnPenDrag({ lastTouchPos.x, lastTouchPos.y });
		        OnPenUp();
		    }
		} else {
			Veci cursPos = Veci{ cursorPositions[0].x, cursorPositions[0].y };
			pen.pressure = cursorPositions[0].pressure;
			if (GetKeyDown(Key::Mouse())) {
			    OnPenDown(cursPos);
			} else {
			    OnPenDrag(cursPos);
			}
		}

		// Handle back button
		if (GetKeyUp(Key::Keyboard(SDL_SCANCODE_AC_BACK)) && backButtonTimer.Time() < 0.3f) {
			Undo();
		} else if (!GetKey(Key::Keyboard(SDL_SCANCODE_AC_BACK))) {
		    backButtonTimer.Restart();
		} else if (backButtonTimer.Time() >= 0.3f) {
		    ClearCanvas();
		}

		pen.Update();

		canvasTex->Draw();

		return true;
	}

	void OnScreenSizeChanged() {
	    Veci oldSize = Veci{ canvasTex->GetWidth(), canvasTex->GetHeight() };
		canvasTex = std::make_unique<Texture>(*this, screenSize.x, screenSize.y);

        auto copy = canvas;
        canvas.clear();
        canvas.resize(screenSize.x * screenSize.y, Colour::Black());
        for (size_t y = 0; y < std::min(screenSize.y, oldSize.y); y++)
            for (size_t x = 0; x < std::min(screenSize.x, oldSize.x); x++)
                canvas[y * screenSize.x + x] = copy[y * oldSize.x + x];

		CommitCanvas();
	}

	void Undo() {
		if (!undoHistory.empty()) {
			auto change = std::move(undoHistory.back());
			undoHistory.pop_back();

			for (const auto&[pos, delta] : change) {
                if (pos.x >= 0 && pos.x < screenSize.x && pos.y >= 0 && pos.y < screenSize.y) {
                    canvas[pos.y * screenSize.x + pos.x] -= delta;
                }
			}
			CommitCanvas();
		}
	}

	void ClearCanvas() {
	    /*undoHistory.emplace_back();

	    for (int y = 0; y < screenSize.y; y++) {
            for (int x = 0; x < screenSize.x; x++) {
                ClearPixel({x, y});
            }
        }

        if (undoHistory.back().empty()) {
            undoHistory.pop_back();
        }*/

        canvas.clear();
        canvas.resize(screenSize.x * screenSize.y, Colour::Black());
        undoHistory.clear();

	    CommitCanvas();
	}

	void OnPenDown(Veci pos) {
		undoHistory.emplace_back();
        pen.pos = pos;
        pen.dragFrom = pos;
	    path.push_back(pos);
	    DrawCircle(pos);
	    CommitCanvas();
	}

	void OnPenDrag(Veci pos) {
	    if (pos != pen.pos) {
            pen.pos = pos;
            path.push_back(pos);

            if (path.size() == 2) {
            	DrawLine(path[0], path[1]);
            }

            if (path.size() == 4) {
                DrawSpline(path[0], path[1], path[2], path[3]);
                CommitCanvas();
                path.pop_front();
            }
        }
	}

	void OnPenUp() {
	    if (path.size() == 2) {
	        DrawLine(path[0], path[1]);
	    } else if (path.size() == 3) {
	        path.push_back(path[2] + (path[2] - path[1]));
	        DrawSpline(path[0], path[1], path[2], path[3]);
	    }
        path.clear();
	    CommitCanvas();

	    if (undoHistory.back().empty()) {
	        undoHistory.pop_back();
	    }
	}

	void CommitCanvas() {
		canvasTex->SetPixels(canvas.data());
	}

	void DrawCircle(Veci origin) {
		if (pen.dragFrom.x < screenSize.x - 100) {
			int radius = (int)((float)pen.radius * pen.EffectivePressure());
			for (int y = -radius; y <= radius; y++)
				for (int x = -radius; x <= radius; x++)
					if (x * x + y * y <= radius * radius) {
						float p = 1.0f;// - (float) (x * x + y * y) / (float) (radius * radius);
						//p *= * pen.EffectivePressure();
						SetPixel(Veci{origin.x + x, origin.y + y}, p);
					}
		}
	}

	void DrawSpline(Veci p1, Veci p2, Veci p3, Veci p4) {
	    std::vector<Veci> pts;
	    for (float t = 0.0f; t <= 1.0f; t += 0.2f) {
            constexpr float L = 0.5f;
            float x = (2 * t * t * t - 3 * t * t + 1) * p2.x
                      + (-2 * t * t * t + 3 * t * t) * p3.x
                      + (t * t * t - 2 * t * t + t) * L * (float)(p3.x - p1.x)
                      + (t * t * t - t * t) * L * (float)(p4.x - p2.x);

            float y = (2 * t * t * t - 3 * t * t + 1) * p2.y
                      + (-2 * t * t * t + 3 * t * t) * p3.y
                      + (t * t * t - 2 * t * t + t) * L * (float)(p3.y - p1.y)
                      + (t * t * t - t * t) * L * (float)(p4.y - p2.y);
            pts.emplace_back((int)x, (int)y);
        }

	    for (size_t i = 0; i < pts.size() - 1; i++) {
	        DrawLine(pts[i], pts[i + 1]);
	    }
	}

	void DrawLine(Veci from, Veci to) {

		if (from == to) {
			DrawCircle(from);
			return;
		}

		auto x1 = (float)from.x;
		auto y1 = (float)from.y;
		auto x2 = (float)to.x;
		auto y2 = (float)to.y;

		float xdiff = (x2 - x1);
		float ydiff = (y2 - y1);

		if(fabs(xdiff) > fabs(ydiff)) {
			float xmin, xmax;

			// set xmin to the lower x value given
			// and xmax to the higher value
			if(x1 < x2) {
				xmin = x1;
				xmax = x2;
			} else {
				xmin = x2;
				xmax = x1;
			}

			// draw line in terms of y slope
			float slope = ydiff / xdiff;
			for(float x = xmin; x <= xmax; x += 1.0f) {
				float y = y1 + ((x - x1) * slope);
				DrawCircle(Veci{ (int)x, (int)y });
			}
		} else {
			float ymin, ymax;

			// set ymin to the lower y value given
			// and ymax to the higher value
			if(y1 < y2) {
				ymin = y1;
				ymax = y2;
			} else {
				ymin = y2;
				ymax = y1;
			}

			// draw line in terms of x slope
			float slope = xdiff / ydiff;
			for(float y = ymin; y <= ymax; y += 1.0f) {
				float x = x1 + ((y - y1) * slope);
				DrawCircle(Veci{ (int)x, (int)y });
			}
		}
	}

	void SetPixel(Veci pos, float pressure = 1.0f) {
		if (pos.x >= 0 && pos.x < screenSize.x && pos.y >= 0 && pos.y < screenSize.y) {
			auto& px = canvas[pos.y * screenSize.x + pos.x];
			auto oldPx = px;
			px.r = std::max(px.r, (uint8_t)((float)pen.colour.r * pressure));
			px.g = std::max(px.g, (uint8_t)((float)pen.colour.g * pressure));
			px.b = std::max(px.b, (uint8_t)((float)pen.colour.b * pressure));
			if (px != oldPx) {
                undoHistory.back()[pos] += px - oldPx;
            }
		}
	}

    void ClearPixel(Veci pos) {
        if (pos.x >= 0 && pos.x < screenSize.x && pos.y >= 0 && pos.y < screenSize.y) {
            auto& px = canvas[pos.y * screenSize.x + pos.x];
            auto oldPx = px;
            px = Colour::Black();
            if (px != oldPx) {
                undoHistory.back()[pos] += px - oldPx;
            }
        }
    }

	Colour GetPixel(Veci pos) {
		return canvas[pos.y * screenSize.x + pos.x];
	}

	Veci screenSize{};
	std::vector<Colour> canvas;
	std::unique_ptr<Texture> canvasTex;
	std::deque<Veci> path;
	std::vector<std::unordered_map<Veci, Colour>> undoHistory;
	Stopwatch<> backButtonTimer;

	struct Pen {
		//Colour colour = Colour{ 0xFF, 0x72, 0x26, 0xFF };
		Colour colour = Colour{ 0xB6, 0xC2, 0xE8, 0xFF };
		int radius = 10;
		Veci pos;
		Veci dragFrom;
		float pressure = 1.0f;
		float effectivePressure = 1.0f;

		void Update() {
		    if (pressure == 1.0f) {
		        effectivePressure = 1.0f;
		    } else {
                effectivePressure = (pressure - effectivePressure) * 0.3f + effectivePressure;
            }
		}

		float EffectivePressure() const {
		    if (effectivePressure == 1.0f) {
		        return 0.35f; // Not using pressure sensitive device
            } else {
                return std::max(0.05f, std::pow(effectivePressure, 0.3f));
		    }
		}
	} pen;
};


int main(int argc, char** argv) {
	try {
		App app;
		app.Run();
		return 0;
	} catch (std::exception& ex) {
		Application::ErrorMessage(ex.what());
		return -1;
	}
}
