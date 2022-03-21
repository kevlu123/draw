#pragma once
#include <algorithm>
#include <string>
#include <cmath>
#include <functional>

template <class T>
struct Vec {
	T x, y;

	constexpr Vec() : Vec((T)0, (T)0) {}
	constexpr Vec(T x, T y) : x(x), y(y) {}

	static constexpr Vec Square(T n) {
		return Vec(n, n);
	}

	static constexpr Vec One() {
		return Vec::Square(1);
	}

	Vec operator+() const {
		return *this;
	}

	Vec operator-() const {
		return Vec{ -x, -y };
	}

	template <class U>
	constexpr Vec<U> Cast() const {
		return Vec<U>(
			static_cast<U>(x),
			static_cast<U>(y)
		);
	}

	Vec HadamMul(const Vec& rhs) const {
		return Vec{ x * rhs.x, y * rhs.y };
	}

	Vec HadamDiv(const Vec& rhs) const {
		return Vec{ x / rhs.x, y / rhs.y };
	}

	static Vec Up() { return Vec(0, 1); }
	static Vec Down() { return Vec(0, -1); }
	static Vec Left() { return Vec(-1, 0); }
	static Vec Right() { return Vec(1, 0); }

	template <class F, class... Args>
	Vec Apply(F f, const Args&... args) const {
		return Vec{
			f(x, (args.x)...),
			f(y, (args.y)...)
		};
	}

	Vec Lerp(const Vec& rhs, float t) const {
		return Vec{
			(T)((rhs.x - x) * t + x),
			(T)((rhs.y - y) * t + y),
		};
	}

	Vec<int> Floor() const {
		return Vec<int>{
			(int)std::floor(x),
			(int)std::floor(y)
		};
	}

	Vec<int> Ceil() const {
		return Vec<int>{
			(int)std::ceil(x),
			(int)std::ceil(y)
		};
	}

	T DistanceSquared() const {
		return x * x + y * y;
	}

	T Distance() const {
		return std::sqrt(DistanceSquared());
	}

	Vec Normalized() const {
		return *this / Distance();
	}

	std::wstring ToString() const {
		return L"(" + std::to_wstring(x) + L", " + std::to_wstring(y) + L")";
	}
};

template <class T>
Vec<T> operator+(const Vec<T>& lhs, const Vec<T>& rhs) {
	return Vec<T>{ lhs.x + rhs.x, lhs.y + rhs.y };
}

template <class T>
Vec<T> operator-(const Vec<T>& lhs, const Vec<T>& rhs) {
	return Vec<T>{ lhs.x - rhs.x, lhs.y - rhs.y };
}

template <class T>
Vec<T> operator*(T lhs, const Vec<T>& rhs) {
	return Vec<T>{ lhs* rhs.x, lhs* rhs.y };
}

template <class T>
Vec<T> operator*(const Vec<T>& lhs, T rhs) {
	return Vec<T>{ lhs.x* rhs, lhs.y* rhs };
}

template <class T>
Vec<T> operator/(const Vec<T>& lhs, T rhs) {
	return Vec<T>{ lhs.x / rhs, lhs.y / rhs };
}

template <class T>
Vec<T>& operator+=(Vec<T>& lhs, const Vec<T>& rhs) {
	lhs = lhs + rhs;
	return lhs;
}

template <class T>
Vec<T>& operator-=(Vec<T>& lhs, const Vec<T>& rhs) {
	lhs = lhs - rhs;
	return lhs;
}

template <class T>
Vec<T>& operator*=(Vec<T>& lhs, T rhs) {
	lhs = lhs * rhs;
	return lhs;
}

template <class T>
Vec<T>& operator/=(Vec<T>& lhs, T rhs) {
	lhs = lhs / rhs;
	return lhs;
}

template <class T>
bool operator==(const Vec<T>& lhs, const Vec<T>& rhs) {
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <class T>
bool operator!=(const Vec<T>& lhs, const Vec<T>& rhs) {
	return !(lhs == rhs);
}

namespace std {
	template<class T>
	struct hash<Vec<T>> {
		size_t operator()(const Vec<T> &v) const {
			return (std::hash<T>()(v.x) >> 1)
				   ^ std::hash<T>()(v.y);
		}
	};
}

using Vecf = Vec<float>;
using Veci = Vec<int>;
